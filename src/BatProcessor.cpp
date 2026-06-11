#include "BatProcessor.h"
#include "BatWorker.h"
#include "BatUtils.h"
#include "BatFFmpegRAII.h"
#include "BatIoScheduler.h" 
#include "CodecWeights.h"
#include "FFmpegMemLoader.h"
#include "StorageTopologyAnalyzer.h"
#include "AudioExtensionList.h"

#include <QMap>
#include <QSet>
#include <QDir>
#include <QFile>
#include <QDebug>
#include <QFileInfo>
#include <QtConcurrent>
#include <QDirIterator>
#include <QElapsedTimer>
#include <cmath>
#include <mutex>
#include <thread>
#include <numeric>
#include <algorithm>


#ifdef Q_OS_WIN
#define NOMINMAX
#include <windows.h>
#include <psapi.h>
#endif

static const int64_t MAX_READ_POOL_BYTES = 4LL * 1024 * 1024 * 1024;
static const int64_t LARGE_FILE_THRESHOLD = 4LL * 1024 * 1024 * 1024;
static const int64_t MAX_WRITE_POOL_BYTES = 512LL * 1024 * 1024;

BatProcessor::BatProcessor(QObject *parent) 
    : QObject(parent)
{
    qRegisterMetaType<FilePerformanceMetrics>("FilePerformanceMetrics");
    m_stopFlag = false;
    m_pauseFlag = false;
    m_totalFileCount = 0;
    m_processedFileCount = 0;
    m_runningWorkers = 0;
    m_bucketIdPadding = 1; 
}

BatProcessor::~BatProcessor(){
    if (m_runningWorkers.load() > 0) {
        stop();
    }
    cleanup();
}

std::optional<BatTask> BatProcessor::claimNextTask(){
    if (m_isHddMode) {
        std::unique_lock<std::mutex> lock(m_ioMutex);
        m_ioCV.wait(lock, [this] {
            return !m_bufferPool.isEmpty() || m_ioFinished || m_stopFlag.load();
        });
        if (m_stopFlag.load()) return std::nullopt;
        if (m_bufferPool.isEmpty()) {
            if (m_ioFinished) return std::nullopt;
            return std::nullopt;
        }
        BatTask task = m_bufferPool.dequeue();
        m_ioCV.notify_all(); 
        return task;
    } 
    else {
        std::lock_guard<std::mutex> lock(m_queueMutex);
        if (m_taskQueue.isEmpty()) {
            return std::nullopt;
        }
        FileInfo info = m_taskQueue.dequeue();
        BatTask task;
        task.info = info;
        task.isLoadedInMemory = false;
        return task;
    }
}

void BatProcessor::notifyLargeFileFinished(){
    std::lock_guard<std::mutex> lock(m_ioMutex);
    if (m_largeFileInProgress) {
        m_largeFileInProgress = false;
        m_ioCV.notify_all();
    }
}

void BatProcessor::releaseReadBuffer(size_t size) {
    m_currentPoolBytes -= size;
    m_ioCV.notify_all();
}

void BatProcessor::enqueueWriteTask(BatWriteTask task) {
    if (task.encodedData.size() > 256 * 1024 * 1024) {
        bool writeSuccess = false;
        QString errorInfo;
        QFile file(task.outputPath);
        if (file.open(QIODevice::WriteOnly)) {
            qint64 written = file.write(task.encodedData);
            if (written == task.encodedData.size()) {
                file.flush();
                file.close();
                writeSuccess = true;
            } else {
                file.close();
                file.remove(); 
                errorInfo = tr("写入不完整，可能是磁盘已满：") + task.outputPath;
            }
        } else {
            errorInfo = tr("无法打开目标文件进行写入：") + task.outputPath;
        }
        QMetaObject::invokeMethod(this, "onAsyncWriteCompleted", Qt::QueuedConnection,
                                  Q_ARG(QString, task.sourceFilePath),
                                  Q_ARG(bool, writeSuccess),
                                  Q_ARG(QString, writeSuccess ? task.successInfo : errorInfo),
                                  Q_ARG(FilePerformanceMetrics, task.metrics),
                                  Q_ARG(int, task.bucketId));
        return; 
    }
    std::unique_lock<std::mutex> lock(m_writeMutex);
    m_writeCV.wait(lock, [this] {
        return (m_currentWritePoolBytes < MAX_WRITE_POOL_BYTES) || m_stopFlag.load();
    });
    if (m_stopFlag.load()) return;
    qint64 size = task.encodedData.size();
    m_writeQueue.enqueue(std::move(task));
    m_currentWritePoolBytes += size;
    m_writeCV.notify_all();
}

void BatProcessor::scanDirectory(const BatSettings& settings) {
    if (m_runningWorkers.load() > 0) {
        emit logMessage(BatUtils::getCurrentTimestamp() + tr(" [错误] 一个任务已在运行中 请等待其完成后再开始新任务"));
        return;
    }
    cleanup();
    m_currentSettings = settings;
    m_stopFlag = false;
    m_pauseFlag = false;
    m_processedFileCount = 0;
    m_failedFiles.clear();
    m_invalidWhiteListFiles.clear();
    m_unexpectedValidFiles.clear();
    m_videoFilesWithAudioExt.clear();
    m_resizedFiles.clear();
    m_allFileMetrics.clear();
    m_bucketCompletionTimes.clear();
    m_pendingFiles.clear();
    emit batchStarted();
    emit logMessage(BatUtils::getCurrentTimestamp() + tr(" 准备开始任务"));
    emit logMessage(BatUtils::formatSettings(settings, true));
    emit logMessage("--------------------------------------------------");
    int threadCount = settings.threadCount;
    if (threadCount <= 0) {
        threadCount = QThread::idealThreadCount();
        if (threadCount <= 0) threadCount = 2;
    }
    m_planFuture = QtConcurrent::run([=]() {
        return BatIoScheduler::analyze(settings.inputPath, settings.outputPath, threadCount);
    });
    emit logMessage(BatUtils::getCurrentTimestamp() + tr(" 初始化任务 开始扫描有效音频文件"));
    QList<FileInfo> validFiles = scanAndProbeFiles(settings.inputPath, settings.includeSubfolders);
    if (m_stopFlag.load()) {
        emit batchStopped();
        return;
    }
    if (validFiles.isEmpty()) {
        emit logMessage(BatUtils::getCurrentTimestamp() + tr(" 未找到任何包含音频流的文件"));
        emit batchFinished(tr("任务完成  但未处理任何文件"));
        return;
    }
    m_pendingFiles = validFiles;
    QSharedPointer<FileSnapshot> snapshot = QSharedPointer<FileSnapshot>::create();
    for (const auto& file : validFiles) {
        qint64 size = QFileInfo(file.path).size();
        QString compositeKey = QString("%1|%2")
            .arg(QFileInfo(file.path).absoluteFilePath().replace('\\', '/'))
            .arg(size);
        snapshot->insert(compositeKey, size);
    }
    emit logMessage(BatUtils::getCurrentTimestamp() + QString(tr(" [扫描完毕] 共找到 %1 个有效音频文件")).arg(validFiles.size()));
    emit scanFinished(snapshot);
}

void BatProcessor::startProcessing(){
    if (m_stopFlag.load()) {
        emit batchStopped();
        return;
    }
    m_totalTaskTimer.start();
    m_totalFileCount = m_pendingFiles.size();
    m_isCleaningUp = false;
    m_workersDone = false;
    int threadCount = m_currentSettings.threadCount;
    if (threadCount <= 0) {
        threadCount = QThread::idealThreadCount();
        if (threadCount <= 0) threadCount = 2;
    }
    BatExecutionPlan plan = m_planFuture.result();
    startWorkers(m_pendingFiles, m_currentSettings, plan);
}

void BatProcessor::startWorkers(QList<FileInfo>& allFiles, const BatSettings& settings, BatExecutionPlan plan) {
    int totalThreadQuota = settings.threadCount;
    if (totalThreadQuota <= 0) {
        totalThreadQuota = QThread::idealThreadCount();
        if (totalThreadQuota <= 0) totalThreadQuota = 2;
    }
    if (totalThreadQuota == 1) {
        plan.ioMode = BatIoThreadMode::None;
        plan.workerDirectWrite = true;
    }
    emit logMessage(BatUtils::getCurrentTimestamp() + " " + plan.strategyName);
    int auxiliaryThreads = 0;
    if (plan.ioMode == BatIoThreadMode::SeparateReadWrite) {
        auxiliaryThreads = 2;
    }
    else if (plan.ioMode != BatIoThreadMode::None) {
        auxiliaryThreads = 1;
    }
    int workerCount = std::max(1, totalThreadQuota - auxiliaryThreads);
    QString threadLog = QString(tr("    [线程分配] 总配额: %1个")).arg(totalThreadQuota);
    threadLog += QString(tr("\n        工作线程: %1个")).arg(workerCount);
    if (plan.ioMode == BatIoThreadMode::ReaderOnly) {
        threadLog += tr("\n        I/O 线程: 1个读取");
    }
    else if (plan.ioMode == BatIoThreadMode::WriterOnly) {
        threadLog += tr("\n        I/O 线程: 1个写入");
    }
    else if (plan.ioMode == BatIoThreadMode::SeparateReadWrite) {
        threadLog += tr("\n        I/O 线程: 1个读取 + 1个写入");
    }
    else if (plan.ioMode == BatIoThreadMode::Hybrid) {
        threadLog += tr("\n        I/O 线程: 1个混合读写");
    }
    if (plan.workerDirectWrite && plan.ioMode != BatIoThreadMode::None) {
        threadLog += tr("\n        读写方式: 工作线程直接读写");
    }
    emit logMessage(threadLog);
    QSet<QString> createdDirs;
    QSet<QString> usedPaths;
    QString inputPathNorm = QDir::cleanPath(QFileInfo(settings.inputPath).absoluteFilePath());
    QDir inputDir(inputPathNorm);
    QDir outputDir(settings.outputPath);
    QString suffix;
    const QString& fmt = settings.exportFormat;
    if (fmt == "PNG" || fmt == "QtPNG") suffix = ".png";
    else if (fmt == "BMP") suffix = ".bmp";
    else if (fmt == "TIFF") suffix = ".tif";
    else if (fmt == "JPG") suffix = ".jpg";
    else if (fmt == "WebP") suffix = ".webp";
    else if (fmt == "AVIF") suffix = ".avif";
    else if (fmt == "JPEG 2000") suffix = ".jp2";
    else suffix = ".png";
    for (auto& file : allFiles) {
        QFileInfo inputFileInfo(file.path);
        QString baseName = inputFileInfo.fileName();
        if (baseName.isEmpty()) baseName = "Unknown";
        QString rootDirName = inputDir.dirName();
        QString subPath = rootDirName;
        if (settings.categorizeByCodec && !file.codecName.isEmpty()) {
            subPath = QDir(subPath).filePath(file.codecName.toUpper());
            subPath = QDir(subPath).filePath(rootDirName);
        }
        if (settings.reuseSubfolderStructure) {
            QString fileAbsPath = QDir::cleanPath(inputFileInfo.absolutePath());
            QString relative = inputDir.relativeFilePath(fileAbsPath);
            if (relative == ".") relative = "";
            if (relative.startsWith("..")) relative = ""; 
            if (!relative.isEmpty()) {
                subPath = QDir(subPath).filePath(relative);
            }
        }
        QString targetDirPath = outputDir.absoluteFilePath(subPath);
        if (!createdDirs.contains(targetDirPath)) {
            QDir().mkpath(targetDirPath);
            createdDirs.insert(targetDirPath);
        }
        QString candidatePath = QDir(targetDirPath).absoluteFilePath(baseName + suffix);
        int counter = 1;
        while (usedPaths.contains(candidatePath)) {
            candidatePath = QDir(targetDirPath).absoluteFilePath(baseName + QString("_%1").arg(counter) + suffix);
            counter++;
        }
        usedPaths.insert(candidatePath);
        file.outputFilePath = candidatePath;
    }
    emit logMessage(BatUtils::getCurrentTimestamp() + tr(" 开始处理\n"));
    if (plan.ioMode == BatIoThreadMode::None || plan.ioMode == BatIoThreadMode::WriterOnly) {
        std::sort(allFiles.begin(), allFiles.end(), [](const FileInfo& a, const FileInfo& b) {
            bool isIsoA = a.path.endsWith(".iso", Qt::CaseInsensitive);
            bool isIsoB = b.path.endsWith(".iso", Qt::CaseInsensitive);
            if (isIsoA != isIsoB) return isIsoA;
            int weightA = CODEC_ID_WEIGHTS.value(a.codecId, DEFAULT_CODEC_WEIGHT);
            int weightB = CODEC_ID_WEIGHTS.value(b.codecId, DEFAULT_CODEC_WEIGHT);
            return (a.duration * weightA) > (b.duration * weightB);
            });
        std::lock_guard<std::mutex> lock(m_queueMutex);
        m_taskQueue.clear();
        for (const auto& file : allFiles) m_taskQueue.enqueue(file);
    }
    m_ioFinished = false;
    m_currentPoolBytes = 0;
    m_largeFileInProgress = false;
    m_bufferPool.clear();
    m_currentWritePoolBytes = 0;
    m_writeQueue.clear();
    if (plan.ioMode == BatIoThreadMode::ReaderOnly || plan.ioMode == BatIoThreadMode::SeparateReadWrite) {
        QThread* runner = QThread::create([this, allFiles]() {
            runReaderLoop(allFiles);
            });
        runner->setParent(this);
        connect(runner, &QThread::finished, this, &BatProcessor::onIoThreadFinished);
        m_ioThread = runner;
        runner->start();
    }
    if (plan.ioMode == BatIoThreadMode::WriterOnly || plan.ioMode == BatIoThreadMode::SeparateReadWrite) {
        QThread* runner = QThread::create([this]() {
            runWriterLoop();
            });
        runner->setParent(this);
        connect(runner, &QThread::finished, this, &BatProcessor::onWriterThreadFinished);
        m_writerThread = runner;
        runner->start();
    }
    if (plan.ioMode == BatIoThreadMode::Hybrid) {
        QThread* runner = QThread::create([this, allFiles]() {
            runHybridLoop(allFiles);
            });
        runner->setParent(this);
        connect(runner, &QThread::finished, this, &BatProcessor::onIoThreadFinished);
        m_ioThread = runner;
        runner->start();
    }
    bool managedRead = (plan.ioMode == BatIoThreadMode::ReaderOnly ||
        plan.ioMode == BatIoThreadMode::SeparateReadWrite ||
        plan.ioMode == BatIoThreadMode::Hybrid);
    m_isHddMode = managedRead;
    m_bucketIdPadding = (workerCount > 0) ? static_cast<int>(std::floor(std::log10(workerCount))) + 1 : 1;
    m_runningWorkers = workerCount;
    for (int i = 0; i < workerCount; ++i) {
        QThread* thread = new QThread(this);
        BatWorker* worker = new BatWorker(i + 1, m_bucketIdPadding, &m_stopFlag, &m_pauseFlag, this);
        worker->setDirectWriteMode(plan.workerDirectWrite);
        worker->moveToThread(thread);
        connect(thread, &QThread::finished, worker, &QObject::deleteLater);
        connect(thread, &QThread::started, this, [=]() {
            QMetaObject::invokeMethod(worker, "startWorkLoop", Qt::QueuedConnection, Q_ARG(BatSettings, settings));
            });
        connect(worker, &BatWorker::fileCompleted, this, &BatProcessor::onWorkerFileCompleted, Qt::QueuedConnection);
        connect(worker, &BatWorker::logMessage, this, &BatProcessor::onWorkerLog, Qt::QueuedConnection);
        connect(worker, &BatWorker::bucketFinished, this, &BatProcessor::onBucketFinished, Qt::QueuedConnection);
        connect(worker, &BatWorker::bucketFinished, thread, &QThread::quit);
        connect(thread, &QThread::finished, this, &BatProcessor::onWorkerFinished, Qt::QueuedConnection);
        connect(worker, &BatWorker::fileMetricsReported, this, &BatProcessor::onFileMetricsReported, Qt::QueuedConnection);
        m_workerThreads.append(thread);
        m_workers.append(worker);
        thread->start();
    }
    if (m_runningWorkers == 0 && !allFiles.isEmpty()) {
        onWorkerFinished();
    }
}

void BatProcessor::runReaderLoop(QList<FileInfo> files){
    for (const auto& info : files) {
        if (m_stopFlag.load()) break;
        while (m_pauseFlag.load() && !m_stopFlag.load()) {
            QThread::msleep(100);
        }
        if (m_stopFlag.load()) break;
        BatTask task;
        task.info = info;
        qint64 fileSize = QFileInfo(info.path).size();
        if (fileSize >= LARGE_FILE_THRESHOLD) {
            std::unique_lock<std::mutex> lock(m_ioMutex);
            task.isLoadedInMemory = false;
            task.memoryData = nullptr;
            m_bufferPool.enqueue(task);
            m_largeFileInProgress = true; 
            m_ioCV.notify_all();
            m_ioCV.wait(lock, [this] {
                return !m_largeFileInProgress || m_stopFlag.load();
            });
        }
        else {
            SharedFileBuffer buffer = FFmpegMemLoader::loadFileToMemory(info.path);
            if (!buffer) {
                task.isLoadedInMemory = false;
            } else {
                task.isLoadedInMemory = true;
                task.memoryData = buffer;
            }
            {
                std::unique_lock<std::mutex> lock(m_ioMutex);
                m_ioCV.wait(lock, [this] {
                    return m_currentPoolBytes < MAX_READ_POOL_BYTES || m_stopFlag.load();
                });
                if (m_stopFlag.load()) break;
                m_bufferPool.enqueue(task);
                if (task.isLoadedInMemory) {
                    m_currentPoolBytes += buffer->size();
                }
                m_ioCV.notify_all();
            }
        }
    }
    {
        std::lock_guard<std::mutex> lock(m_ioMutex);
        m_ioFinished = true;
        m_ioCV.notify_all();
    }
}

void BatProcessor::runWriterLoop() {
    while (true) {
        BatWriteTask task;
        bool hasTask = false;
        {
            std::unique_lock<std::mutex> lock(m_writeMutex);
            m_writeCV.wait(lock, [this] {
                return !m_writeQueue.isEmpty() || m_stopFlag.load() || m_workersDone.load();
            });
            if (m_stopFlag.load()) break;
            if (!m_writeQueue.isEmpty()) {
                task = m_writeQueue.dequeue();
                hasTask = true;
            } else if (m_workersDone.load()) {
                break;
            }
        }
        if (hasTask) {
            bool writeSuccess = false;
            QString errorInfo;
            QFile file(task.outputPath);
            if (file.open(QIODevice::WriteOnly)) {
                qint64 written = file.write(task.encodedData);
                if (written == task.encodedData.size()) {
                    file.flush();
                    file.close();
                    writeSuccess = true;
                } else {
                    file.close();
                    file.remove(); 
                    errorInfo = tr("写入不完整，可能是磁盘已满：") + task.outputPath;
                }
            } else {
                errorInfo = tr("无法打开目标文件写入：") + task.outputPath;
            }
            {
                std::lock_guard<std::mutex> lock(m_writeMutex);
                m_currentWritePoolBytes -= task.encodedData.size();
                m_writeCV.notify_all();
            }
            QMetaObject::invokeMethod(this, "onAsyncWriteCompleted", Qt::QueuedConnection,
                                      Q_ARG(QString, task.sourceFilePath),
                                      Q_ARG(bool, writeSuccess),
                                      Q_ARG(QString, writeSuccess ? task.successInfo : errorInfo),
                                      Q_ARG(FilePerformanceMetrics, task.metrics),
                                      Q_ARG(int, task.bucketId));
        }
    }
}

void BatProcessor::runHybridLoop(QList<FileInfo> files) {
    int fileIndex = 0;
    const int totalFiles = files.size();
    const int64_t WRITE_BATCH_TRIGGER = 64LL * 1024 * 1024; 
    bool isReadCompleted = false; 
    while (!m_stopFlag.load()) {
        bool shouldWrite = false;
        bool isQueueEmpty = false;
        {
            std::lock_guard<std::mutex> wLock(m_writeMutex);
            isQueueEmpty = m_writeQueue.isEmpty();
            if (!isQueueEmpty) {
                if (m_currentWritePoolBytes > WRITE_BATCH_TRIGGER || isReadCompleted || m_workersDone.load()) {
                    shouldWrite = true;
                }
            }
        }
        if (m_workersDone.load() && isQueueEmpty && isReadCompleted) {
            break; 
        }
        if (shouldWrite) {
            while (!m_stopFlag.load()) {
                BatWriteTask task;
                bool hasTask = false;
                {
                    std::unique_lock<std::mutex> lock(m_writeMutex);
                    if (!m_writeQueue.isEmpty()) {
                        task = m_writeQueue.dequeue();
                        hasTask = true;
                    }
                }
                if (!hasTask) break; 
                bool writeSuccess = false;
                QString errorInfo;
                QFile file(task.outputPath);
                if (file.open(QIODevice::WriteOnly)) {
                    qint64 written = file.write(task.encodedData);
                    if (written == task.encodedData.size()) {
                        file.flush();
                        file.close();
                        writeSuccess = true;
                    } else {
                        file.close();
                        file.remove();
                        errorInfo = tr("写入不完整，可能是磁盘已满：") + task.outputPath;
                    }
                } else {
                    errorInfo = tr("无法写入文件：") + task.outputPath;
                }
                {
                    std::lock_guard<std::mutex> lock(m_writeMutex);
                    m_currentWritePoolBytes -= task.encodedData.size();
                    m_writeCV.notify_all();
                }
                QMetaObject::invokeMethod(this, "onAsyncWriteCompleted", Qt::QueuedConnection,
                                          Q_ARG(QString, task.sourceFilePath),
                                          Q_ARG(bool, writeSuccess),
                                          Q_ARG(QString, writeSuccess ? task.successInfo : errorInfo),
                                          Q_ARG(FilePerformanceMetrics, task.metrics),
                                          Q_ARG(int, task.bucketId));
            }
            continue; 
        }
        if (fileIndex < totalFiles) {
            const auto& info = files[fileIndex];
            qint64 fileSize = QFileInfo(info.path).size();
            if (fileSize >= LARGE_FILE_THRESHOLD) {
                BatTask task;
                task.info = info;
                task.isLoadedInMemory = false;
                {
                    std::unique_lock<std::mutex> lock(m_ioMutex);
                    m_bufferPool.enqueue(task);
                    m_largeFileInProgress = true;
                    m_ioCV.notify_all();
                    m_ioCV.wait(lock, [this] { return !m_largeFileInProgress || m_stopFlag.load(); });
                }
            } 
            else {
                SharedFileBuffer buffer = FFmpegMemLoader::loadFileToMemory(info.path);
                BatTask task;
                task.info = info;
                if (buffer) {
                    task.isLoadedInMemory = true;
                    task.memoryData = buffer;
                } else {
                    task.isLoadedInMemory = false;
                }
                {
                    std::unique_lock<std::mutex> lock(m_ioMutex);
                    if (m_currentPoolBytes >= MAX_READ_POOL_BYTES) {
                        m_ioCV.wait_for(lock, std::chrono::milliseconds(100), [this] {
                            return m_currentPoolBytes < MAX_READ_POOL_BYTES || m_stopFlag.load();
                        });
                        if (m_currentPoolBytes >= MAX_READ_POOL_BYTES) {
                            continue;
                        }
                    }
                    if (m_stopFlag.load()) break;
                    m_bufferPool.enqueue(task);
                    if (task.isLoadedInMemory) m_currentPoolBytes += buffer->size();
                    m_ioCV.notify_all();
                }
            }
            fileIndex++;
        } 
        else {
            if (!isReadCompleted) {
                std::lock_guard<std::mutex> lock(m_ioMutex);
                m_ioFinished = true;
                m_ioCV.notify_all();
                isReadCompleted = true;
            }
            if (!shouldWrite) {
                QThread::msleep(50);
            }
        }
    }
    {
        std::lock_guard<std::mutex> lock(m_ioMutex);
        m_ioFinished = true;
        m_ioCV.notify_all();
    }
}

QList<FileInfo> BatProcessor::scanAndProbeFiles(const QString& rootPath, bool includeSubfolders){
    QList<FileInfo> validFiles;
    QDirIterator::IteratorFlags flags = includeSubfolders ? 
                                        QDirIterator::Subdirectories : 
                                        QDirIterator::NoIteratorFlags;
    QDirIterator it(rootPath, QDir::Files, flags);
    QElapsedTimer scanTimer;
    scanTimer.start();
    while (it.hasNext()) {
        if (m_stopFlag.load()) break;
        while (m_pauseFlag.load() && !m_stopFlag.load()) {
            QThread::msleep(100);
        }
        if (m_stopFlag.load()) break;
        QString path = it.next();
        QString suffix = QFileInfo(path).suffix().toLower();
        bool inGlobalList = AudioExtensionList::getDefaultExtensions().contains(suffix);
        bool isVideo = false;
        if (m_currentSettings.enableWhitelist) {
            if (!m_currentSettings.whitelistExtensions.contains(suffix, Qt::CaseInsensitive)) {
                continue;
            }
            auto fileInfoOpt = probeFile(path, isVideo);
            if (fileInfoOpt.has_value()) {
                validFiles.append(fileInfoOpt.value());
            } else {
                std::lock_guard<std::mutex> lock(m_listMutex);
                if (isVideo) m_videoFilesWithAudioExt.append(path);
                else m_invalidWhiteListFiles.append(path);
            }
        } else {
            auto fileInfoOpt = probeFile(path, isVideo);
            if (fileInfoOpt.has_value()) {
                validFiles.append(fileInfoOpt.value());
                if (!inGlobalList) {
                    std::lock_guard<std::mutex> lock(m_listMutex);
                    m_unexpectedValidFiles.append(path);
                }
            } else {
                if (inGlobalList) {
                    std::lock_guard<std::mutex> lock(m_listMutex);
                    if (isVideo) m_videoFilesWithAudioExt.append(path);
                    else m_invalidWhiteListFiles.append(path);
                }
            }
        }
        if (scanTimer.elapsed() >= 10000) {
            emit logMessage(BatUtils::getCurrentTimestamp() + tr(" [正在扫描] 已经扫描到 %1 个有效音频文件").arg(validFiles.size()));
            scanTimer.restart();
        }
    }
    return validFiles;
}

std::optional<FileInfo> BatProcessor::probeFile(const QString& path, bool& outIsVideo){
    outIsVideo = false;
    AVFormatContext* format_ctx_raw = nullptr;
    if (avformat_open_input(&format_ctx_raw, path.toUtf8().constData(), nullptr, nullptr) != 0) {
        return std::nullopt;
    }
    BatFFmpeg::BatFormatContextPtr formatContext(format_ctx_raw);
    if (avformat_find_stream_info(formatContext.get(), nullptr) < 0) {
        return std::nullopt;
    }
    bool hasRealVideo = false;
    for (unsigned int i = 0; i < formatContext->nb_streams; ++i) {
        AVStream* st = formatContext->streams[i];
        if (st->codecpar->codec_type == AVMEDIA_TYPE_VIDEO) {
            if (!(st->disposition & AV_DISPOSITION_ATTACHED_PIC)) {
                hasRealVideo = true;
                break;
            }
        }
    }
    if (hasRealVideo) {
        outIsVideo = true;
        if (m_currentSettings.excludeVideoFiles) {
            return std::nullopt;
        }
    }
    int stream_index = av_find_best_stream(formatContext.get(), AVMEDIA_TYPE_AUDIO, -1, -1, nullptr, 0);
    if (stream_index < 0) {
        return std::nullopt;
    }
    AVStream* stream = formatContext->streams[stream_index];
    const AVCodec* codec = avcodec_find_decoder(stream->codecpar->codec_id);
    if (!codec) {
        return std::nullopt;
    }
    int64_t durationMs = 0;
    bool durationFound = false;
    if (codec->id == AV_CODEC_ID_AAC) {
        BatFFmpeg::BatPacketPtr pkt = BatFFmpeg::make_packet();
        int64_t maxPts = 0;
        bool readAnything = false;
        av_seek_frame(formatContext.get(), stream_index, 0, AVSEEK_FLAG_BACKWARD);
        while (av_read_frame(formatContext.get(), pkt.get()) >= 0) {
            if (pkt->stream_index == stream_index && pkt->pts != AV_NOPTS_VALUE) {
                int64_t endPts = pkt->pts + pkt->duration;
                if (endPts > maxPts) maxPts = endPts;
                readAnything = true;
            }
            av_packet_unref(pkt.get());
        }
        if (readAnything && maxPts > 0) {
            durationMs = static_cast<int64_t>(maxPts * av_q2d(stream->time_base) * 1000);
            durationFound = true;
        }
    }
    else if (codec->id == AV_CODEC_ID_MP3) {
        int64_t hugeTimestamp = (stream->duration != AV_NOPTS_VALUE) ? stream->duration : (1LL << 60);
        if (av_seek_frame(formatContext.get(), stream_index, hugeTimestamp, AVSEEK_FLAG_BACKWARD) >= 0) {
            BatFFmpeg::BatPacketPtr pkt = BatFFmpeg::make_packet();
            int64_t maxPts = 0;
            bool readAnything = false;
            while (av_read_frame(formatContext.get(), pkt.get()) >= 0) {
                if (pkt->stream_index == stream_index && pkt->pts != AV_NOPTS_VALUE) {
                    int64_t endPts = pkt->pts + pkt->duration;
                    if (endPts > maxPts) maxPts = endPts;
                    readAnything = true;
                }
                av_packet_unref(pkt.get());
            }
            if (readAnything && maxPts > 0) {
                durationMs = static_cast<int64_t>(maxPts * av_q2d(stream->time_base) * 1000);
                durationFound = true;
            }
        }
    }
    if (!durationFound) {
        if (formatContext->duration != AV_NOPTS_VALUE) {
            durationMs = formatContext->duration / 1000;
        } else if (stream->duration != AV_NOPTS_VALUE) {
            durationMs = static_cast<int64_t>(stream->duration * av_q2d(stream->time_base) * 1000);
        } else {
            durationMs = 0;
        }
    }
    FileInfo info;
    info.path = path;
    info.duration = durationMs;
    info.codecName = codec->name;
    info.codecId = codec->id;
    return info;
}

void BatProcessor::pause() { m_pauseFlag = true; emit logMessage(BatUtils::getCurrentTimestamp() + tr(" 任务已暂停")); emit batchPaused(); }

void BatProcessor::resume() { m_pauseFlag = false; emit logMessage(BatUtils::getCurrentTimestamp() + tr(" 任务已恢复")); emit batchResumed(); }

void BatProcessor::stop() { 
    m_stopFlag = true; 
    m_ioCV.notify_all();
    m_writeCV.notify_all(); 
    if (m_pauseFlag.load()) { resume(); } 
    emit logMessage(BatUtils::getCurrentTimestamp() + tr(" 正在终止任务 请稍候")); 
}

void BatProcessor::onAsyncWriteCompleted(const QString& filePath, bool success, const QString& errorInfo, const FilePerformanceMetrics& metrics, int threadId) {
    onFileMetricsReported(metrics);
    onWorkerFileCompleted(filePath, success, errorInfo, metrics, threadId);
}

void BatProcessor::onWorkerFileCompleted(const QString& filePath, bool success, const QString& errorInfo, const FilePerformanceMetrics& metrics, int threadId) {
    int current = ++m_processedFileCount;
    int total = m_totalFileCount.load();
    if (success && errorInfo == "WARNING_RESIZED") {
        std::lock_guard<std::mutex> locker(m_resizedFilesMutex);
        m_resizedFiles.append(filePath);
    }
    else if (!success) {
        std::lock_guard<std::mutex> locker(m_failedFilesMutex);
        m_failedFiles.append(filePath);
    }
    QString fileName = QFileInfo(filePath).fileName();
    QString timestamp = BatUtils::getCurrentTimestamp();
    if (success) {
        QString head = QString(tr("%1 [%2/%3] [线程 %4] 成功：%5"))
            .arg(timestamp)
            .arg(current)
            .arg(total)
            .arg(threadId, m_bucketIdPadding, 10, QChar('0'))
            .arg(fileName);
        if (errorInfo == "WARNING_RESIZED") {
            head += tr(" (已自动压缩)");
        }
        emit logMessage(head);
        QString details = QString(tr("                [耗时] 音频解码%1毫秒 傅里叶变换%2毫秒 渲染与保存%3毫秒\n"))
            .arg(metrics.decodingTimeMs)
            .arg(metrics.fftTimeMs)
            .arg(metrics.renderingTimeMs);
        emit logMessage(details);
    }
    else {
        QString head = QString(tr("%1 [%2/%3] [线程 %4] 失败：%5 (原因：%6)\n"))
            .arg(timestamp)
            .arg(current)
            .arg(total)
            .arg(threadId, m_bucketIdPadding, 10, QChar('0'))
            .arg(fileName)
            .arg(errorInfo);
        emit logMessage(head);
    }
    emit progressUpdated(current, total);
}

void BatProcessor::onWorkerLog(const QString& message) { emit logMessage(message); }

void BatProcessor::onBucketFinished(int bucketId, qint64 elapsedMs) { std::lock_guard<std::mutex> lock(m_bucketDataMutex); m_bucketCompletionTimes[bucketId] = elapsedMs; }

void BatProcessor::onFileMetricsReported(const FilePerformanceMetrics& metrics) {
    std::lock_guard<std::mutex> lock(m_metricsMutex);
    m_allFileMetrics.append(metrics);
}

void BatProcessor::checkAndFinishBatch() {
    if (m_runningWorkers.load() > 0) return;
    bool ioThreadRunning = (m_ioThread && m_ioThread->isRunning());
    bool writerThreadRunning = (m_writerThread && m_writerThread->isRunning());
    if (ioThreadRunning || writerThreadRunning) {
        return;
    }
    if (!m_isCleaningUp.exchange(true)) {
        bool wasStoppedByUser = m_stopFlag.load();
        qint64 elapsedMs = m_totalTaskTimer.elapsed();
        QString totalTimeStr = BatUtils::formatElapsedMs(elapsedMs);
        QString timestamp = BatUtils::getCurrentTimestamp();
        emit logMessage("----------------------------------------");
        {
            std::lock_guard<std::mutex> lock(m_bucketDataMutex);
            QMapIterator<int, qint64> i(m_bucketCompletionTimes);
            while (i.hasNext()) {
                i.next();
                if (i.value() > 0) {
                    QString timeStr = BatUtils::formatElapsedMs(i.value());
                    emit logMessage(QString(tr("    [线程%1] 耗时 %2"))
                        .arg(i.key(), m_bucketIdPadding, 10, QChar('0'))
                        .arg(timeStr));
                }
            }
        }
        emit logMessage(timestamp + tr(" 耗时总结"));
        qint64 totalDecode = 0, totalFft = 0, totalRender = 0;
        for (const auto& metrics : m_allFileMetrics) {
            totalDecode += metrics.decodingTimeMs;
            totalFft += metrics.fftTimeMs;
            totalRender += metrics.renderingTimeMs;
        }
        emit logMessage(QString(tr("    音频解码累计耗时：%1秒")).arg(totalDecode / 1000.0, 0, 'f', 3));
        emit logMessage(QString(tr("    傅里叶变换累计耗时：%1秒")).arg(totalFft / 1000.0, 0, 'f', 3));
        emit logMessage(QString(tr("    渲染与保存累计耗时：%1秒")).arg(totalRender / 1000.0, 0, 'f', 3));
        QString summary;
        if (wasStoppedByUser) {
            summary = tr("任务已终止");
            emit logMessage(BatUtils::getCurrentTimestamp() + tr(" 任务结束"));
            emit logMessage(QString(tr("    任务总耗时：%1")).arg(totalTimeStr));
            emit batchStopped();
        }
        else {
            summary = tr("所有任务已处理完毕\n");
            if (m_failedFiles.isEmpty()) {
                summary += tr("所有文件均处理成功\n");
            }
            else {
                summary += QString(tr("\n\n共有 %1 个文件处理失败：\n")).arg(m_failedFiles.size());
                for (const QString& file : m_failedFiles) {
                    summary += file + "\n";
                }
            }
            if (!m_resizedFiles.isEmpty()) {
                summary += QString(tr("\n[提示] 共有 %1 个文件因图像尺寸超限被自动压缩：\n")).arg(m_resizedFiles.size());
                for (const QString& file : m_resizedFiles) {
                    summary += file + "\n";
                }
            }
            if (!m_invalidWhiteListFiles.isEmpty()) {
                summary += QString(tr("\n[提示] 共有 %1 个文件具有音频扩展名，未探测到音频流：\n")).arg(m_invalidWhiteListFiles.size());
                for (const QString& file : m_invalidWhiteListFiles) {
                    summary += file + "\n";
                }
            }
            if (!m_videoFilesWithAudioExt.isEmpty()) {
                summary += QString(tr("\n[提示] 共有 %1 个文件具有音频扩展名，但探测到视频流，未探测到音频流：\n")).arg(m_videoFilesWithAudioExt.size());
                for (const QString& file : m_videoFilesWithAudioExt) {
                    summary += file + "\n";
                }
            }
            if (!m_unexpectedValidFiles.isEmpty()) {
                summary += QString(tr("\n[提示] 共有 %1 个文件不具有音频扩展名，但探测到音频流，已进行处理：\n")).arg(m_unexpectedValidFiles.size());
                for (const QString& file : m_unexpectedValidFiles) {
                    summary += file + "\n";
                }
            }
            emit logMessage(BatUtils::getCurrentTimestamp() + tr(" 任务结束"));
            emit logMessage(QString(tr("    任务总耗时：%1")).arg(totalTimeStr));
            emit batchFinished(summary);
        }
        cleanup();
    }
}

void BatProcessor::onWorkerFinished() {
    if (--m_runningWorkers <= 0) {
        m_workersDone = true;
        m_writeCV.notify_all();
        m_ioCV.notify_all();
        
        checkAndFinishBatch();
    }
}

void BatProcessor::onIoThreadFinished() { 
    checkAndFinishBatch(); 
}

void BatProcessor::onWriterThreadFinished() { 
    checkAndFinishBatch(); 
}

void BatProcessor::cleanup() {
    m_stopFlag = true; 
    m_ioCV.notify_all();
    m_writeCV.notify_all();
    auto safeCleanup = [this](QThread*& thread) {
        if (thread) {
            thread->disconnect(this);
            if (thread->isRunning()) {
                thread->quit();
                if (!thread->wait(2000)) {
                    thread->setParent(nullptr);
                    QObject::connect(thread, &QThread::finished, thread, &QObject::deleteLater);
                    thread = nullptr;
                    return;
                }
            }
            thread->deleteLater();
            thread = nullptr;
        }
    };
    safeCleanup(m_ioThread);
    safeCleanup(m_writerThread);
    for (QThread* thread : m_workerThreads) {
        if (thread) {
            thread->disconnect(this);
            if (thread->isRunning()) {
                thread->quit();
                if (!thread->wait(2000)) {
                    thread->setParent(nullptr);
                    QObject::connect(thread, &QThread::finished, thread, &QObject::deleteLater);
                    continue;
                }
            }
            thread->deleteLater();
        }
    }
    m_workerThreads.clear();
    m_workers.clear();
    m_runningWorkers = 0;
    m_failedFiles = QStringList();
    m_resizedFiles = QStringList();
    m_allFileMetrics = QList<FilePerformanceMetrics>();
    m_taskQueue = QQueue<FileInfo>();
    m_bufferPool = QQueue<BatTask>();
    m_currentPoolBytes = 0;
    m_writeQueue = QQueue<BatWriteTask>();
    m_currentWritePoolBytes = 0;

#ifdef Q_OS_WIN
    HANDLE hHeap = ::GetProcessHeap();
    if (hHeap) {
        ::HeapCompact(hHeap, 0);
    }
#endif

}
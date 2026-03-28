#include "BatchStreamProcessor.h"
#include "BatchStreamWorker.h"
#include "BatchStreamUtils.h"
#include "BatchStreamFFmpegRAII.h"
#include "StorageUtils.h"
#include "CodecWeights.h"

#include <QDirIterator>
#include <QFileInfo>
#include <QDebug>
#include <QFile>
#include <cmath>
#include <algorithm>
#include <numeric>
#include <cstdio>

#ifdef Q_OS_WIN
#define NOMINMAX
#include <windows.h>
#include <psapi.h>
#endif

static const qint64 MAX_WRITE_QUEUE_BYTES = 512LL * 1024 * 1024;

BatchStreamProcessor::BatchStreamProcessor(QObject *parent)
    : QObject(parent)
{
    m_stopFlag = false;
    m_pauseFlag = false;
    m_totalFileCount = 0;
    m_processedFileCount = 0;
    m_activeThreadCount = 0;
    m_bucketIdPadding = 1;
}

BatchStreamProcessor::~BatchStreamProcessor(){
    if (m_activeThreadCount.load() > 0) {
        stop();
    }
    cleanupThreads();
}

void BatchStreamProcessor::scanDirectory(const BatchStreamSettings& settings) {
    if (m_activeThreadCount.load() > 0) {
        emit logMessage(BatchStreamUtils::getCurrentTimestamp() + tr(" [错误] 一个任务已在运行中 请等待其完成后再开始新任务"));
        return;
    }
    cleanupThreads();
    m_currentSettings = settings;
    m_stopFlag = false;
    m_pauseFlag = false;
    m_processedFileCount = 0;
    m_failedFiles.clear();
    m_resizedFiles.clear();
    m_pendingFiles.clear();
    m_bucketCompletionTimes.clear();
    m_totalTimer.start();
    emit batchStarted();
    emit logMessage("--------------------------------------------------");
    emit logMessage(BatchStreamUtils::getCurrentTimestamp() + tr(" [处理模式] 流式"));
    QString subfolderStr = settings.includeSubfolders ? tr("是") : tr("否");
    QString structureStr = settings.reuseSubfolderStructure ? tr("是") : tr("否");
    emit logMessage(tr("    [输入] %1").arg(settings.inputPath));
    emit logMessage(tr("        扫描子文件夹：%1").arg(subfolderStr));
    emit logMessage(tr("    [输出] %1").arg(settings.outputPath));
    emit logMessage(tr("        保持目录结构：%1").arg(structureStr));
    QString widthLimitStr = settings.enableWidthLimit ? QString::number(settings.maxWidth) + tr(" 像素") : tr("未启用");
    QString gridStr = settings.enableGrid ? tr("开") : tr("关");
    QString compStr = settings.enableComponents ? tr("开") : tr("关");
    emit logMessage(tr("    [图像] 高度: %1 像素 | 限宽: %2 | 网格: %3 | 组件: %4")
        .arg(settings.imageHeight).arg(widthLimitStr).arg(gridStr).arg(compStr));
    QString precisionStr = (settings.timeInterval <= 0.000000001) ? tr("自动") : QString::number(settings.timeInterval) + tr(" 秒");
    QString winName = WindowFunctions::getName(settings.windowType);
    QString curveName = MappingCurves::getName(settings.curveType);
    QString paletteName = ColorPalette::getPaletteNames().value(settings.paletteType, tr("未知"));
    emit logMessage(tr("    [算法] 精度: %1 | 窗口: %2 | 映射: %3 | 配色: %4")
        .arg(precisionStr).arg(winName).arg(curveName).arg(paletteName));
    emit logMessage(tr("    [参数] dB范围: %1 ~ %2").arg(settings.minDb).arg(settings.maxDb));
    if (settings.exportFormat == "QtPNG" || settings.exportFormat == "BMP" || settings.exportFormat == "TIFF") {
        emit logMessage(tr("    [导出] 格式: %1").arg(settings.exportFormat));
    } else {
        emit logMessage(tr("    [导出] 格式: %1 | 质量/压缩: %2").arg(settings.exportFormat).arg(settings.qualityLevel));
    }
    emit logMessage("--------------------------------------------------");
    emit logMessage(BatchStreamUtils::getCurrentTimestamp() + tr(" 初始化任务 开始扫描有效音频文件"));
    QList<BatchStreamScannedFile> validFiles = performScan(settings.inputPath, settings.includeSubfolders);
    if (m_stopFlag.load()) {
        emit batchStopped();
        return;
    }
    if (validFiles.isEmpty()) {
        emit logMessage(BatchStreamUtils::getCurrentTimestamp() + tr(" 未找到任何包含音频流的文件"));
        emit batchFinished(tr("任务完成  但未处理任何文件"));
        return;
    }
    m_pendingFiles = validFiles;
    QSharedPointer<BatchStreamFileSnapshot> snapshot = QSharedPointer<BatchStreamFileSnapshot>::create();
    for (const auto& file : validFiles) {
        QString compositeKey = QString("%1|%2")
            .arg(QFileInfo(file.path).absoluteFilePath().replace('\\', '/'))
            .arg(file.fileSize);
        snapshot->insert(compositeKey, file.fileSize);
    }
    emit logMessage(BatchStreamUtils::getCurrentTimestamp() + QString(tr(" 扫描完毕 共找到%1个有效音频文件")).arg(validFiles.size()));
    emit scanFinished(snapshot);
}

void BatchStreamProcessor::startProcessing(){
    if (m_stopFlag.load()) {
        emit batchStopped();
        return;
    }
    m_totalFileCount = m_pendingFiles.size();
    startWorkers(m_pendingFiles);
}

QList<BatchStreamScannedFile> BatchStreamProcessor::performScan(const QString& rootPath, bool includeSubfolders){
    QList<BatchStreamScannedFile> validFiles;
    QDirIterator::IteratorFlags flags = includeSubfolders ? QDirIterator::Subdirectories : QDirIterator::NoIteratorFlags;
    QDirIterator it(rootPath, QDir::Files | QDir::NoDotAndDotDot, flags);
    while (it.hasNext()) {
        if (m_stopFlag.load()) break;
        QString path = it.next();
        auto infoOpt = probeFile(path);
        if (infoOpt.has_value()) validFiles.append(infoOpt.value());
    }
    return validFiles;
}

std::optional<BatchStreamScannedFile> BatchStreamProcessor::probeFile(const QString& path){
    AVFormatContext* format_ctx_raw = nullptr;
    if (avformat_open_input(&format_ctx_raw, path.toUtf8().constData(), nullptr, nullptr) != 0) return std::nullopt;
    BatchStreamFFmpeg::FormatContextPtr formatContext(format_ctx_raw);
    if (avformat_find_stream_info(formatContext.get(), nullptr) < 0) return std::nullopt;
    int stream_index = av_find_best_stream(formatContext.get(), AVMEDIA_TYPE_AUDIO, -1, -1, nullptr, 0);
    if (stream_index < 0) return std::nullopt;
    AVStream* stream = formatContext->streams[stream_index];
    const AVCodec* codec = avcodec_find_decoder(stream->codecpar->codec_id);
    if (!codec) return std::nullopt;
    qint64 durationMs = 0;
    bool durationFound = false;
    if (codec->id == AV_CODEC_ID_AAC) {
        BatchStreamFFmpeg::PacketPtr pkt(av_packet_alloc());
        int64_t maxPts = 0;
        bool readAny = false;
        av_seek_frame(formatContext.get(), stream_index, 0, AVSEEK_FLAG_BACKWARD);
        while (av_read_frame(formatContext.get(), pkt.get()) >= 0) {
            if (pkt->stream_index == stream_index && pkt->pts != AV_NOPTS_VALUE) {
                int64_t endPts = pkt->pts + pkt->duration;
                if (endPts > maxPts) maxPts = endPts;
                readAny = true;
            }
            av_packet_unref(pkt.get());
        }
        if (readAny && maxPts > 0) {
            durationMs = static_cast<qint64>(maxPts * av_q2d(stream->time_base) * 1000);
            durationFound = true;
        }
    }
    else if (codec->id == AV_CODEC_ID_MP3) {
        int64_t hugeTimestamp = (stream->duration != AV_NOPTS_VALUE) ? stream->duration : (1LL << 60);
        if (av_seek_frame(formatContext.get(), stream_index, hugeTimestamp, AVSEEK_FLAG_BACKWARD) >= 0) {
            BatchStreamFFmpeg::PacketPtr pkt(av_packet_alloc());
            int64_t maxPts = 0;
            bool readAny = false;
            while (av_read_frame(formatContext.get(), pkt.get()) >= 0) {
                if (pkt->stream_index == stream_index && pkt->pts != AV_NOPTS_VALUE) {
                    int64_t endPts = pkt->pts + pkt->duration;
                    if (endPts > maxPts) maxPts = endPts;
                    readAny = true;
                }
                av_packet_unref(pkt.get());
            }
            if (readAny && maxPts > 0) {
                durationMs = static_cast<qint64>(maxPts * av_q2d(stream->time_base) * 1000);
                durationFound = true;
            }
        }
    }
    if (!durationFound) {
        if (formatContext->duration != AV_NOPTS_VALUE) durationMs = formatContext->duration / 1000;
        else if (stream->duration != AV_NOPTS_VALUE) durationMs = static_cast<qint64>(stream->duration * av_q2d(stream->time_base) * 1000);
    }
    BatchStreamScannedFile info;
    info.path = path;
    info.duration = durationMs;
    info.codecName = codec->name;
    info.codecId = codec->id;
    info.fileSize = QFileInfo(path).size();
    return info;
}

void BatchStreamProcessor::startWorkers(QList<BatchStreamScannedFile>& allFiles) {
    int totalThreadQuota = m_currentSettings.threadCount;
    if (totalThreadQuota <= 0) {
        if (m_currentSettings.useMultiThreading) {
            int hardware = QThread::idealThreadCount();
            totalThreadQuota = (hardware > 0) ? hardware : 2;
        }
        else {
            totalThreadQuota = 1;
        }
    }
    BatchStreamExecutionPlan plan = BatchStreamIoScheduler::analyze(
        m_currentSettings.inputPath,
        m_currentSettings.outputPath,
        totalThreadQuota
    );
    if (totalThreadQuota == 1) {
        plan.ioMode = BatchStreamIoThreadMode::None;
        plan.workerDirectWrite = true;
    }
    m_ioMode = plan.ioMode;
    int auxiliaryThreads = 0;
    if (m_ioMode == BatchStreamIoThreadMode::SeparateReadWrite) {
        auxiliaryThreads = 2;
    }
    else if (m_ioMode != BatchStreamIoThreadMode::None) {
        auxiliaryThreads = 1;
    }
    int workerCount = std::max(1, totalThreadQuota - auxiliaryThreads);
    m_bucketIdPadding = (workerCount > 0) ? static_cast<int>(std::floor(std::log10(workerCount))) + 1 : 1;
    emit logMessage(BatchStreamUtils::getCurrentTimestamp() + " " + plan.strategyName);
    if (!plan.strategyDescription.isEmpty()) {
        emit logMessage(tr("        %1").arg(plan.strategyDescription));
    }
    QString threadLog = QString(tr("    [线程分配] 总配额: %1个")).arg(totalThreadQuota);
    threadLog += QString(tr("\n        工作线程: %1个")).arg(workerCount);

    if (m_ioMode == BatchStreamIoThreadMode::ReaderOnly) {
        threadLog += tr("\n        I/O 线程: 1个读取");
    }
    else if (m_ioMode == BatchStreamIoThreadMode::WriterOnly) {
        threadLog += tr("\n        I/O 线程: 1个写入");
    }
    else if (m_ioMode == BatchStreamIoThreadMode::SeparateReadWrite) {
        threadLog += tr("\n        I/O 线程: 1个读取 + 1个写入");
    }
    else if (m_ioMode == BatchStreamIoThreadMode::Hybrid) {
        threadLog += tr("\n        I/O 线程: 1个混合读写");
    }
    if (plan.workerDirectWrite && m_ioMode != BatchStreamIoThreadMode::None) {
        threadLog += tr("\n        读写方式: 工作线程直接读写");
    }
    emit logMessage(threadLog);
    emit logMessage(BatchStreamUtils::getCurrentTimestamp() + tr(" 开始处理\n"));
    m_ioFinished = false;
    m_readyTasks.clear();
    m_writeQueue.clear();
    m_currentWriteBytes = 0;
    if (m_ioMode == BatchStreamIoThreadMode::None || m_ioMode == BatchStreamIoThreadMode::WriterOnly) {
        std::sort(allFiles.begin(), allFiles.end(), [](const BatchStreamScannedFile& a, const BatchStreamScannedFile& b) {
            bool isIsoA = a.path.endsWith(".iso", Qt::CaseInsensitive);
            bool isIsoB = b.path.endsWith(".iso", Qt::CaseInsensitive);
            if (isIsoA != isIsoB) {
                return isIsoA;
            }
            int weightA = CODEC_ID_WEIGHTS.value(static_cast<AVCodecID>(a.codecId), DEFAULT_CODEC_WEIGHT);
            int weightB = CODEC_ID_WEIGHTS.value(static_cast<AVCodecID>(b.codecId), DEFAULT_CODEC_WEIGHT);
            return (a.duration * weightA) > (b.duration * weightB);
            });
        std::lock_guard<std::mutex> lock(m_taskMutex);
        m_ssdTaskQueue.clear();
        for (const auto& f : allFiles) {
            m_ssdTaskQueue.enqueue(f);
        }
    }
    bool needReader = (m_ioMode == BatchStreamIoThreadMode::ReaderOnly ||
        m_ioMode == BatchStreamIoThreadMode::SeparateReadWrite ||
        m_ioMode == BatchStreamIoThreadMode::Hybrid);
    bool needWriter = (m_ioMode == BatchStreamIoThreadMode::WriterOnly ||
        m_ioMode == BatchStreamIoThreadMode::SeparateReadWrite);
    if (needReader) {
        if (m_ioMode == BatchStreamIoThreadMode::Hybrid) {
            m_ioThread = QThread::create([this, allFiles, workerCount]() {
                runHybridLoop(allFiles, workerCount);
                });
        }
        else {
            m_ioThread = QThread::create([this, allFiles, workerCount]() {
                runReaderLoop(allFiles, workerCount);
                });
        }
        m_ioThread->setParent(this);
        connect(m_ioThread, &QThread::finished, this, &BatchStreamProcessor::onIoThreadFinished);
        m_ioThread->start();
    }
    if (needWriter) {
        m_writerThread = QThread::create([this]() {
            runWriterLoop();
            });
        m_writerThread->setParent(this);
        connect(m_writerThread, &QThread::finished, this, &BatchStreamProcessor::onWriterThreadFinished);
        m_writerThread->start();
    }
    m_activeThreadCount = 0;
    for (int i = 0; i < workerCount; ++i) {
        QThread* thread = new QThread(this);
        BatchStreamWorker* worker = new BatchStreamWorker(
            i + 1,
            m_bucketIdPadding,
            &m_stopFlag,
            &m_pauseFlag,
            this
        );
        worker->setDirectWriteMode(plan.workerDirectWrite);
        worker->moveToThread(thread);
        connect(thread, &QThread::finished, worker, &QObject::deleteLater);
        connect(thread, &QThread::started, worker, [worker, this, settings = m_currentSettings]() {
            worker->setProcessor(this);
            worker->startWorkLoop(settings);
            });
        connect(worker, &BatchStreamWorker::fileFinished, this, &BatchStreamProcessor::onWorkerFileFinished);
        connect(worker, &BatchStreamWorker::logMessage, this, &BatchStreamProcessor::onWorkerLog);
        connect(worker, &BatchStreamWorker::bucketFinished, this, &BatchStreamProcessor::onWorkerBucketFinished);
        connect(worker, &BatchStreamWorker::bucketFinished, thread, &QThread::quit);
        connect(thread, &QThread::finished, this, &BatchStreamProcessor::onWorkerThreadFinished);
        m_threads.append(thread);
        m_workers.append(worker);
        m_activeThreadCount++;
        thread->start();
    }
}

void BatchStreamProcessor::enqueueWriteTask(BatchStreamWriteTask task) {
    if (task.encodedData.size() > 256 * 1024 * 1024) {
        std::unique_lock<std::mutex> lock(m_writeMutex);
        QFile file(task.outputPath);
        if (file.open(QIODevice::WriteOnly)) {
            file.write(task.encodedData);
            file.close();
        }
        return; 
    }
    std::unique_lock<std::mutex> lock(m_writeMutex);
    m_writeCond.wait(lock, [this] {
        return (m_currentWriteBytes < MAX_WRITE_QUEUE_BYTES) || m_stopFlag.load();
    });
    if (m_stopFlag.load()) return;
    qint64 size = task.encodedData.size();
    m_writeQueue.enqueue(std::move(task));
    m_currentWriteBytes += size;
    m_writeCond.notify_all();
}

void BatchStreamProcessor::runReaderLoop(QList<BatchStreamScannedFile> files, int concurrency) {
    struct ActiveContext {
        FILE* fp;
        std::shared_ptr<BatchStreamBuffer> buffer; 
        QString path;
    };
    std::vector<ActiveContext> activeContexts;
    activeContexts.reserve(concurrency);
    QQueue<BatchStreamScannedFile> pendingQueue;
    for(const auto& f : files) pendingQueue.enqueue(f);
    const size_t IO_BLOCK_SIZE = 4 * 1024 * 1024; 
    std::vector<uint8_t> readBuffer(IO_BLOCK_SIZE);
    while (!m_stopFlag.load()) {
        bool idleCycle = true;
        while (activeContexts.size() < static_cast<size_t>(concurrency) && !pendingQueue.isEmpty()) {
            if (m_stopFlag.load()) break;
            BatchStreamScannedFile fileInfo = pendingQueue.dequeue();
            FILE* fp = nullptr;
#ifdef _WIN32
            fp = _wfopen(fileInfo.path.toStdWString().c_str(), L"rb");
#else
            fp = fopen(fileInfo.path.toUtf8().constData(), "rb");
#endif
            if (!fp) {
                emit onWorkerFileFinished(fileInfo.path, false, tr("无法打开文件"), 0, 0);
                continue;
            }
            auto buffer = std::make_shared<BatchStreamBuffer>(32 * 1024 * 1024);
            {
                std::lock_guard<std::mutex> lock(m_taskMutex);
                BatchStreamTask task;
                task.fileInfo = fileInfo;
                task.buffer = buffer;
                m_readyTasks.enqueue(task);
            }
            m_taskCond.notify_one();
            activeContexts.push_back({fp, buffer, fileInfo.path});
            idleCycle = false;
        }
        for (int i = activeContexts.size() - 1; i >= 0; --i) {
            if (m_stopFlag.load()) break;
            auto& ctx = activeContexts[i];
            if ((ctx.buffer->capacity() - ctx.buffer->bytesAvailable()) > IO_BLOCK_SIZE) {
                size_t bytesRead = fread(readBuffer.data(), 1, IO_BLOCK_SIZE, ctx.fp);
                bool writeOk = true;
                if (bytesRead > 0) {
                    writeOk = ctx.buffer->write(readBuffer.data(), bytesRead);
                    idleCycle = false;
                }
                if (!writeOk || bytesRead < IO_BLOCK_SIZE) {
                    if (writeOk && bytesRead < IO_BLOCK_SIZE) ctx.buffer->setEof();
                    fclose(ctx.fp);
                    activeContexts.erase(activeContexts.begin() + i);
                }
            }
        }
        if (activeContexts.empty() && pendingQueue.isEmpty()) break;
        if (idleCycle) QThread::msleep(10); 
        while (m_pauseFlag.load() && !m_stopFlag.load()) QThread::msleep(100);
    }
    for (auto& ctx : activeContexts) {
        if (ctx.fp) fclose(ctx.fp);
        ctx.buffer->abort();
    }
    {
        std::lock_guard<std::mutex> lock(m_taskMutex);
        m_ioFinished = true;
    }
    m_taskCond.notify_all();
}

void BatchStreamProcessor::runWriterLoop() {
    while (!m_stopFlag.load()) {
        BatchStreamWriteTask task;
        bool hasTask = false;
        {
            std::unique_lock<std::mutex> lock(m_writeMutex);
            m_writeCond.wait(lock, [this] {
                return !m_writeQueue.isEmpty() || m_stopFlag.load();
            });
            if (m_stopFlag.load() && m_writeQueue.isEmpty()) break;
            if (!m_writeQueue.isEmpty()) {
                task = m_writeQueue.dequeue();
                m_currentWriteBytes -= task.encodedData.size();
                hasTask = true;
                m_writeCond.notify_all();
            }
        }
        if (hasTask) {
            QFile file(task.outputPath);
            if (file.open(QIODevice::WriteOnly)) {
                file.write(task.encodedData);
                file.close();
            } else {
                emit logMessage(tr("无法写入文件: ") + task.outputPath);
            }
        }
    }
}

void BatchStreamProcessor::runHybridLoop(QList<BatchStreamScannedFile> files, int concurrency) {
    struct ActiveContext {
        FILE* fp;
        std::shared_ptr<BatchStreamBuffer> buffer; 
        QString path;
    };
    std::vector<ActiveContext> activeContexts;
    activeContexts.reserve(concurrency);
    QQueue<BatchStreamScannedFile> pendingQueue;
    for(const auto& f : files) pendingQueue.enqueue(f);
    const size_t IO_BLOCK_SIZE = 4 * 1024 * 1024; 
    std::vector<uint8_t> readBuffer(IO_BLOCK_SIZE);
    const qint64 WRITE_BATCH_TRIGGER = 32LL * 1024 * 1024; 
    bool isReadPhaseFinished = false;
    while (!m_stopFlag.load()) {
        bool shouldWrite = false;
        {
            std::lock_guard<std::mutex> wLock(m_writeMutex);
            if (!m_writeQueue.isEmpty()) {
                if (m_currentWriteBytes > WRITE_BATCH_TRIGGER || isReadPhaseFinished) {
                    shouldWrite = true;
                }
            }
        }
        if (shouldWrite) {
            while (!m_stopFlag.load()) {
                BatchStreamWriteTask task;
                bool hasTask = false;
                {
                    std::unique_lock<std::mutex> lock(m_writeMutex);
                    if (!m_writeQueue.isEmpty()) {
                        task = m_writeQueue.dequeue();
                        m_currentWriteBytes -= task.encodedData.size();
                        hasTask = true;
                        m_writeCond.notify_all(); 
                    }
                }
                if (!hasTask) break;
                QFile file(task.outputPath);
                if (file.open(QIODevice::WriteOnly)) {
                    file.write(task.encodedData);
                    file.close();
                } else {
                    emit logMessage(tr("无法写入文件: ") + task.outputPath);
                }
            }
            if (!isReadPhaseFinished) {
            } else {
               QThread::msleep(10);
               continue;
            }
        }
        bool idleCycle = true;
        while (activeContexts.size() < static_cast<size_t>(concurrency) && !pendingQueue.isEmpty()) {
            if (m_stopFlag.load()) break;
            BatchStreamScannedFile fileInfo = pendingQueue.dequeue();
            FILE* fp = nullptr;
#ifdef _WIN32
            fp = _wfopen(fileInfo.path.toStdWString().c_str(), L"rb");
#else
            fp = fopen(fileInfo.path.toUtf8().constData(), "rb");
#endif
            if (!fp) {
                emit onWorkerFileFinished(fileInfo.path, false, tr("无法打开文件"), 0, 0);
                continue;
            }
            auto buffer = std::make_shared<BatchStreamBuffer>(32 * 1024 * 1024);
            {
                std::lock_guard<std::mutex> lock(m_taskMutex);
                BatchStreamTask task;
                task.fileInfo = fileInfo;
                task.buffer = buffer;
                m_readyTasks.enqueue(task);
            }
            m_taskCond.notify_one();
            activeContexts.push_back({fp, buffer, fileInfo.path});
            idleCycle = false;
        }
        for (int i = activeContexts.size() - 1; i >= 0; --i) {
            if (m_stopFlag.load()) break;
            auto& ctx = activeContexts[i];
            if ((ctx.buffer->capacity() - ctx.buffer->bytesAvailable()) > IO_BLOCK_SIZE) {
                size_t bytesRead = fread(readBuffer.data(), 1, IO_BLOCK_SIZE, ctx.fp);
                bool writeOk = true;
                if (bytesRead > 0) {
                    writeOk = ctx.buffer->write(readBuffer.data(), bytesRead);
                    idleCycle = false;
                }
                if (!writeOk || bytesRead < IO_BLOCK_SIZE) {
                    if (writeOk && bytesRead < IO_BLOCK_SIZE) ctx.buffer->setEof();
                    fclose(ctx.fp);
                    activeContexts.erase(activeContexts.begin() + i);
                }
            }
        }
        if (activeContexts.empty() && pendingQueue.isEmpty()) {
            if (!isReadPhaseFinished) {
                isReadPhaseFinished = true;
                {
                    std::lock_guard<std::mutex> lock(m_taskMutex);
                    m_ioFinished = true;
                }
                m_taskCond.notify_all();
            }
        }
        if (idleCycle && !shouldWrite) {
            QThread::msleep(10);
        }
        while (m_pauseFlag.load() && !m_stopFlag.load()) {
            QThread::msleep(100);
        }
    }
    for (auto& ctx : activeContexts) {
        if (ctx.fp) fclose(ctx.fp);
        ctx.buffer->abort();
    }
    {
        std::lock_guard<std::mutex> lock(m_taskMutex);
        m_ioFinished = true;
    }
    m_taskCond.notify_all();
}

std::optional<BatchStreamTask> BatchStreamProcessor::claimTask(int workerId) {
    bool needReader = (m_ioMode == BatchStreamIoThreadMode::ReaderOnly || 
                       m_ioMode == BatchStreamIoThreadMode::SeparateReadWrite || 
                       m_ioMode == BatchStreamIoThreadMode::Hybrid);
    if (needReader) {
        std::unique_lock<std::mutex> lock(m_taskMutex);
        m_taskCond.wait(lock, [this]() {
            return !m_readyTasks.isEmpty() || m_ioFinished || m_stopFlag.load();
        });
        if (m_stopFlag.load()) return std::nullopt;
        if (!m_readyTasks.isEmpty()) {
            return m_readyTasks.dequeue();
        }
        if (m_ioFinished) return std::nullopt;
        return std::nullopt;
    } 
    else {
        std::lock_guard<std::mutex> lock(m_taskMutex);
        if (m_ssdTaskQueue.isEmpty()) return std::nullopt;
        BatchStreamTask task;
        task.fileInfo = m_ssdTaskQueue.dequeue();
        task.buffer = nullptr;
        return task;
    }
}

void BatchStreamProcessor::onIoThreadFinished() {}

void BatchStreamProcessor::onWriterThreadFinished() {}

void BatchStreamProcessor::pause() { 
    m_pauseFlag = true; 
    emit logMessage(BatchStreamUtils::getCurrentTimestamp() + tr(" 任务已暂停"));
    emit batchPaused(); 
}

void BatchStreamProcessor::resume() { 
    m_pauseFlag = false; 
    emit logMessage(BatchStreamUtils::getCurrentTimestamp() + tr(" 任务已恢复"));
    emit batchResumed(); 
}

void BatchStreamProcessor::stop() { 
    m_stopFlag = true; 
    m_taskCond.notify_all();
    m_writeCond.notify_all();
    if (m_pauseFlag.load()) { resume(); } 
    emit logMessage(BatchStreamUtils::getCurrentTimestamp() + tr(" 正在终止任务 请稍候"));
}

void BatchStreamProcessor::onWorkerFileFinished(const QString& filePath, bool success, const QString& errorInfo, int threadId, qint64 elapsedMs){
    int current = ++m_processedFileCount;
    int total = m_totalFileCount.load();
    QString fileName = QFileInfo(filePath).fileName();
    QString timeStr = BatchStreamUtils::formatElapsedTime(elapsedMs);
    if (success) {
        QString extraInfo = "";
        if (errorInfo == "WARNING_RESIZED") {
            extraInfo = tr("(已自动压缩)");
            std::lock_guard<std::mutex> locker(m_resizedFilesMutex);
            m_resizedFiles.append(filePath);
        }
        emit logMessage(QString(tr("%1 [%2/%3] [线程 %4] [耗时 %5] 成功：%6 %7\n"))
            .arg(BatchStreamUtils::getCurrentTimestamp())
            .arg(current)
            .arg(total)
            .arg(threadId, m_bucketIdPadding, 10, QChar('0'))
            .arg(timeStr)
            .arg(fileName)
            .arg(extraInfo)
            );
    }
    else {
        std::lock_guard<std::mutex> locker(m_mutex);
        m_failedFiles.append(filePath);
        emit logMessage(QString(tr("%1 [%2/%3] [线程 %4] 失败：%5\n"))
            .arg(BatchStreamUtils::getCurrentTimestamp())
            .arg(current)
            .arg(total)
            .arg(threadId, m_bucketIdPadding, 10, QChar('0'))
            .arg(fileName)
            .arg(errorInfo)
            );
    }
    emit progressUpdated(current, total);
}

void BatchStreamProcessor::onWorkerLog(const QString& message) { emit logMessage(message); }

void BatchStreamProcessor::onWorkerBucketFinished(int bucketId, qint64 elapsedMs){
    std::lock_guard<std::mutex> lock(m_bucketDataMutex);
    m_bucketCompletionTimes[bucketId] = elapsedMs;
}

void BatchStreamProcessor::onWorkerThreadFinished(){
    m_activeThreadCount--;
    if (m_activeThreadCount <= 0) {
        qint64 elapsedMs = m_totalTimer.elapsed();
        QString totalTimeStr = BatchStreamUtils::formatElapsedTime(elapsedMs);
        QString timestamp = BatchStreamUtils::getCurrentTimestamp();
        emit logMessage("----------------------------------------");
        {
            std::lock_guard<std::mutex> lock(m_bucketDataMutex);
            QMapIterator<int, qint64> i(m_bucketCompletionTimes);
            while (i.hasNext()) {
                i.next();
                QString timeStr = BatchStreamUtils::formatElapsedTime(i.value());
                emit logMessage(QString(tr("    [线程 %1] 耗时 %2"))
                    .arg(i.key(), m_bucketIdPadding, 10, QChar('0'))
                    .arg(timeStr));
            }
        }
        QString summary;
        if (m_stopFlag.load()) {
            summary = tr("任务已终止");
        } else {
            summary = tr("所有任务已处理完毕\n");
            if (m_failedFiles.isEmpty()) {
                summary += tr("所有文件均处理成功");
            } else {
                summary += QString(tr("共%1个文件处理失败：\n")).arg(m_failedFiles.size());
                for (const QString& file : m_failedFiles) {
                    summary += file + "\n";
                }
            }
            if (!m_resizedFiles.isEmpty()) {
                summary += QString(tr("\n\n[提示] 共%1个文件因尺寸超限被自动压缩：\n")).arg(m_resizedFiles.size());
                for (const QString& file : m_resizedFiles) {
                    summary += file + "\n";
                }
            }
        }
        emit logMessage(QString(tr("    总计耗时：%1")).arg(totalTimeStr));
        if (m_stopFlag.load()) { emit batchStopped(); } else { emit batchFinished(summary); }
        cleanupThreads();
    }
}

void BatchStreamProcessor::cleanupThreads(){
    m_stopFlag = true;
    m_taskCond.notify_all();
    m_writeCond.notify_all();
    if (m_ioThread) {
        if (m_ioThread->isRunning()) {
            m_ioThread->quit();
            m_ioThread->wait(5000);
        }
        delete m_ioThread;
        m_ioThread = nullptr;
    }
    if (m_writerThread) {
        if (m_writerThread->isRunning()) {
            m_writerThread->quit();
            m_writerThread->wait(5000);
        }
        delete m_writerThread;
        m_writerThread = nullptr;
    }
    for (QThread* thread : m_threads) {
        if (thread->isRunning()) {
            thread->quit();
            thread->wait(5000);
        }
    }
    qDeleteAll(m_threads);
    m_workers.clear();
    m_threads.clear();
    m_activeThreadCount = 0;
    m_failedFiles = QStringList();
    m_resizedFiles = QStringList();
    m_readyTasks = QQueue<BatchStreamTask>();
    m_ssdTaskQueue = QQueue<BatchStreamScannedFile>();
    m_writeQueue = QQueue<BatchStreamWriteTask>();
    m_currentWriteBytes = 0;
#ifdef Q_OS_WIN
    HANDLE hHeap = ::GetProcessHeap();
    if (hHeap) {
        ::HeapCompact(hHeap, 0);
    }
#endif
}
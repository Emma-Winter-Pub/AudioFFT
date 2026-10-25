#include "BPBS_Processor.h"
#include "BPBS_Worker.h"
#include "StorageTopologyAnalyzer.h"
#include "TextEncodingHelper.h"

#include <QtConcurrent>
#include <QFileInfo>
#include <QDateTime>
#include <algorithm>

extern "C" {
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libavutil/dict.h>
}

#ifdef Q_OS_WIN
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#endif
#ifdef __linux__
#include <malloc.h>
#endif

BPBS_Processor::BPBS_Processor(QObject *parent)
    : QObject(parent)
    , m_demandQueue((std::max)(1, QThread::idealThreadCount() - 4))
{
    m_workerPool = std::make_unique<QThreadPool>();
    m_workerPool->setMaxThreadCount(m_demandQueue.maxWorkers());
    m_workerPool->setExpiryTimeout(-1);
    m_debounceTimer = new QTimer(this);
    m_debounceTimer->setSingleShot(true);
    m_debounceTimer->setInterval(300);
    connect(m_debounceTimer, &QTimer::timeout, this, &BPBS_Processor::onDebounceTimeout);
    m_ioReaderThread = QThread::create([this]() { runIoReaderLoop(); });
    m_ioReaderThread->setParent(this);
    m_ioReaderThread->start(QThread::LowPriority);
    m_metaProbeThread = QThread::create([this]() { runMetadataProbeLoop(); });
    m_metaProbeThread->setParent(this);
    m_metaProbeThread->start(QThread::LowestPriority);
}

BPBS_Processor::~BPBS_Processor() {
    cleanupThreads();
}

void BPBS_Processor::setFolderFileList(const QString& domainId, const QList<BPBS_DemandItem>& allFiles) {
    QString cleanDomain = QDir::cleanPath(domainId);
    cleanDomain.replace('\\', '/');
    if (!m_currentDomain.isEmpty() && m_currentDomain != cleanDomain) {
        m_cache.onDomainLeave(m_currentDomain, QDateTime::currentMSecsSinceEpoch(), m_demandQueue.currentB());
    }
    m_demandQueue.clearAndAbortAll();
    m_currentDomain = cleanDomain;
    m_cache.setCurrentDomain(cleanDomain);
    m_metaCache.setCurrentDomain(cleanDomain);
    m_demandQueue.setFolderFileList(allFiles);
    m_isFastStorage.store(false, std::memory_order_relaxed);
    m_demandQueue.setFastStorage(false);
    m_ioReaderCv.notify_all();
    (void)QtConcurrent::run([this, cleanDomain]() {
        VolumeTopology topo = StorageTopologyAnalyzer::analyze(cleanDomain);
        bool isSsd = (topo.hasSSD && !topo.hasHDD && !topo.isNetworkStorage);
        m_isFastStorage.store(isSsd, std::memory_order_relaxed);
        m_demandQueue.setFastStorage(isSsd);
        m_ioReaderCv.notify_all();
    });
    {
        std::lock_guard<std::mutex> lock(m_metaProbeMutex);
        m_metaProbeQueue.clear();
        for (const auto& item : allFiles) {
            m_metaProbeQueue.push_back(item.filePath);
        }
    }
    m_metaProbeCv.notify_all();
}

void BPBS_Processor::requestViewportUpdate(int firstRow, int lastRow) {
    m_pendingFirstRow = firstRow;
    m_pendingLastRow = lastRow;
    if (m_debounceTimer) {
        m_debounceTimer->start();
    }
}

void BPBS_Processor::onDebounceTimeout() {
    uint64_t currentFingerprint = 0;
    {
        std::lock_guard<std::mutex> lock(m_settingsMutex);
        currentFingerprint = m_currentCalcParams.toFingerprint();
    }
    m_demandQueue.updateViewportRange(m_pendingFirstRow, m_pendingLastRow, m_cache, m_metaCache, currentFingerprint);
    QList<BPBS_DemandItem> currentB = m_demandQueue.currentB();
    {
        std::lock_guard<std::mutex> lock(m_metaProbeMutex);
        std::deque<QString> newProbeQueue;
        for (const auto& item : currentB) {
            newProbeQueue.push_back(item.filePath);
        }
        m_metaProbeQueue = std::move(newProbeQueue);
    }
    m_metaProbeCv.notify_all();
    m_ioReaderCv.notify_all();
}

void BPBS_Processor::updatePreviewSettings(const BPBS_Settings& settings) {
    std::lock_guard<std::mutex> lock(m_settingsMutex);
    m_currentCalcParams = settings.calcParams();
    m_currentVisualParams = settings.visualParams();
    if (m_syncMode && m_debounceTimer) {
        m_debounceTimer->start();
    }
}

void BPBS_Processor::updateForegroundMetadata(const QString& filePath, const BPBS_AudioMetadata& meta) {
    if (m_metaCache.tryUpgrade(filePath, meta.fileSizeBytes, MetadataPriority::ForegroundFocus, meta)) {
        emit metadataReady(filePath, meta);
    }
}

void BPBS_Processor::updatePalette(const QString& paletteId, bool inverted, bool negative) {
    {
        std::lock_guard<std::mutex> lock(m_settingsMutex);
        m_currentVisualParams.paletteId = paletteId;
        m_currentVisualParams.paletteInverted = inverted;
        m_currentVisualParams.paletteNegative = negative;
    }
    const auto& colors = ColorPaletteFactory::instance().getPalette(paletteId, inverted, negative);
    m_cache.updateAllColorTables(colors);
    QStringList activePaths = m_cache.getAllPaths();
    for (const QString& path : activePaths) {
        emit thumbnailReady(path);
    }
}

BPBS_Settings BPBS_Processor::currentSettings() const {
    std::lock_guard<std::mutex> lock(m_settingsMutex);
    BPBS_Settings s;
    return s;
}

QString BPBS_Processor::currentDomain() const {
    return m_currentDomain;
}

BPBS_PreemptResult BPBS_Processor::requestPreempt(const QString& filePath) {
    bool isFast = m_isFastStorage.load(std::memory_order_relaxed);
    return m_demandQueue.preemptP1(filePath, isFast);
}

void BPBS_Processor::clearP1File() {
    m_demandQueue.clearP1File();
}

void BPBS_Processor::freezeForExport() {
    m_demandQueue.setPaused(true);
}

void BPBS_Processor::unfreezeFromExport() {
    m_demandQueue.setPaused(false);
}

void BPBS_Processor::startPreviewWorkers() {
    if (m_stopFlag.load(std::memory_order_relaxed)) {
        return;
    }
    m_stopFlag.store(false, std::memory_order_relaxed);
    m_pauseFlag.store(false, std::memory_order_relaxed);
    auto calcGetter = [this]() -> BPBS_CalcParams {
        std::lock_guard<std::mutex> lock(m_settingsMutex);
        return m_currentCalcParams;
    };
    auto visualGetter = [this]() -> BPBS_VisualParams {
        std::lock_guard<std::mutex> lock(m_settingsMutex);
        return m_currentVisualParams;
    };
    for (int i = 0; i < m_demandQueue.maxWorkers(); ++i) {
        int workerId = i + 1;
        int padding = static_cast<int>(std::floor(std::log10(m_demandQueue.maxWorkers()))) + 1;
        auto* worker = new BPBS_Worker(
            workerId,
            padding,
            m_demandQueue,
            m_cache,
            m_metaCache,
            calcGetter,
            visualGetter,
            &m_stopFlag,
            &m_pauseFlag
        );
        connect(worker, &BPBS_Worker::thumbnailReady, this, &BPBS_Processor::thumbnailReady, Qt::QueuedConnection);
        connect(worker, &BPBS_Worker::metadataReady, this, &BPBS_Processor::metadataReady, Qt::QueuedConnection);
        connect(worker, &BPBS_Worker::fileNotAudio, this, &BPBS_Processor::fileNotAudio, Qt::QueuedConnection);
        connect(worker, &BPBS_Worker::p1FileReleased, this, &BPBS_Processor::p1FileReleased, Qt::QueuedConnection);
        connect(worker, &BPBS_Worker::logMessage, this, &BPBS_Processor::logMessage, Qt::QueuedConnection);
        m_workerPool->start(worker);
    }
}

void BPBS_Processor::pause() {
    m_pauseFlag.store(true, std::memory_order_relaxed);
    m_demandQueue.setPaused(true);
}

void BPBS_Processor::resume() {
    m_pauseFlag.store(false, std::memory_order_relaxed);
    m_demandQueue.setPaused(false);
}

void BPBS_Processor::stop() {
    m_stopFlag.store(true, std::memory_order_relaxed);
    m_demandQueue.stop();
    m_ioReaderCv.notify_all();
    m_metaProbeCv.notify_all();
}

void BPBS_Processor::cleanupThreads() {
    stop();
    if (m_debounceTimer) m_debounceTimer->stop();
    m_ioReaderCv.notify_all();
    if (m_ioReaderThread) {
        if (m_ioReaderThread->isRunning()) {
            m_ioReaderThread->quit();
            m_ioReaderThread->wait();
        }
        m_ioReaderThread->deleteLater();
        m_ioReaderThread = nullptr;
    }
    if (m_metaProbeThread) {
        if (m_metaProbeThread->isRunning()) {
            m_metaProbeThread->quit();
            m_metaProbeThread->wait();
        }
        m_metaProbeThread->deleteLater();
        m_metaProbeThread = nullptr;
    }
    if (m_workerPool) {
        m_workerPool->waitForDone();
    }
#ifdef Q_OS_WIN
    HANDLE hHeap = ::GetProcessHeap();
    if (hHeap) {
        ::HeapCompact(hHeap, 0);
    }
#elif defined(__linux__)
    QThreadPool* pool = QThreadPool::globalInstance();
    int maxThreads = pool->maxThreadCount();
    pool->setMaxThreadCount(0);
    QThread::msleep(50);
    pool->setMaxThreadCount(maxThreads);
    malloc_trim(0);
#endif
}

void BPBS_Processor::setMetadataProbingPaused(bool visible) {
    m_metaProbePaused.store(!visible, std::memory_order_relaxed);
    if (visible) {
        m_metaProbeCv.notify_all();
    }
}

void BPBS_Processor::runMetadataProbeLoop() {
    while (!m_stopFlag.load(std::memory_order_relaxed)) {
        QString filePath;
        {
            std::unique_lock<std::mutex> lock(m_metaProbeMutex);
            m_metaProbeCv.wait(lock, [this]() {
                return (!m_metaProbeQueue.empty() && !m_metaProbePaused.load(std::memory_order_relaxed))
                        || m_stopFlag.load(std::memory_order_relaxed);
            });
            if (m_stopFlag.load(std::memory_order_relaxed)) break;
            filePath = m_metaProbeQueue.front();
            m_metaProbeQueue.pop_front();
        }
        if (filePath.isEmpty()) continue;
        qint64 fileSize = QFileInfo(filePath).size();
        MetadataPriority currentPriority = m_metaCache.getPriority(filePath, fileSize);
        if (currentPriority != MetadataPriority::Unprobed) {
            continue;
        }
        probeSingleFileMetadata(filePath);
    }
}

void BPBS_Processor::probeSingleFileMetadata(const QString& filePath) {
    qint64 fileSize = QFileInfo(filePath).size();
    if (fileSize == 0) {
        m_metaCache.markInvalidAudio(filePath, fileSize);
        emit fileNotAudio(filePath);
        m_demandQueue.markCompleted(filePath);
        return;
    }
    AVFormatContext* formatCtxRaw = avformat_alloc_context();
    if (!formatCtxRaw) return;
    formatCtxRaw->probesize = 5 * 1024 * 1024;
    formatCtxRaw->max_analyze_duration = 2 * AV_TIME_BASE;
    if (avformat_open_input(&formatCtxRaw, filePath.toUtf8().constData(), nullptr, nullptr) != 0) {
        avformat_free_context(formatCtxRaw);
        m_metaCache.markInvalidAudio(filePath, fileSize);
        emit fileNotAudio(filePath);
        m_demandQueue.markCompleted(filePath);
        return;
    }
    std::unique_ptr<AVFormatContext, void(*)(AVFormatContext*)> formatCtx(
        formatCtxRaw, [](AVFormatContext* ctx) { if (ctx) avformat_close_input(&ctx); }
    );
    if (avformat_find_stream_info(formatCtx.get(), nullptr) < 0) {
        m_metaCache.markInvalidAudio(filePath, fileSize);
        emit fileNotAudio(filePath);
        m_demandQueue.markCompleted(filePath);
        return;
    }
    int audioStreamIdx = av_find_best_stream(formatCtx.get(), AVMEDIA_TYPE_AUDIO, -1, -1, nullptr, 0);
    if (audioStreamIdx < 0) {
        m_metaCache.markInvalidAudio(filePath, fileSize);
        emit fileNotAudio(filePath);
        m_demandQueue.markCompleted(filePath);
        return;
    }
    AVStream* stream = formatCtx->streams[audioStreamIdx];
    const AVCodec* codec = avcodec_find_decoder(stream->codecpar->codec_id);
    BPBS_AudioMetadata info;
    info.filePath = filePath;
    info.fileSizeBytes = fileSize;
    info.totalAudioBytes = 0;
    info.totalSamples = 0;
    info.defaultTrackIndex = audioStreamIdx;
    info.sampleRateHz = stream->codecpar->sample_rate;
    int bits = stream->codecpar->bits_per_raw_sample;
    if (bits <= 0) bits = av_get_bytes_per_sample(static_cast<AVSampleFormat>(stream->codecpar->format)) * 8;
    if (bits <= 0) bits = 16;
    info.sourceBitDepth = bits;
    info.bitrateBps = formatCtx->bit_rate;
    info.preciseBitrateBps = 0;
    info.formatName = formatCtx->iformat->name;
    info.formatLongName = formatCtx->iformat->long_name ? formatCtx->iformat->long_name : info.formatName;
    info.codecName = codec ? QString(codec->name).toUpper() : "UNKNOWN";
    info.codecLongName = codec && codec->long_name ? codec->long_name : info.codecName;
    info.codecId = codec ? codec->id : 0;
    double durationSec = 0.0;
    if (formatCtx->duration != AV_NOPTS_VALUE) {
        durationSec = static_cast<double>(formatCtx->duration) / AV_TIME_BASE;
    } else if (stream->duration != AV_NOPTS_VALUE) {
        durationSec = stream->duration * av_q2d(stream->time_base);
    }
    info.durationSeconds = durationSec;
    info.preciseDurationSeconds = 0.0;
    info.durationMicroseconds = static_cast<long long>(durationSec * 1000000.0);
    auto readTag = [&](const char* key, QString& outStr) {
        AVDictionaryEntry* tag = av_dict_get(formatCtx->metadata, key, nullptr, 0);
        if (tag && strlen(tag->value) > 0) {
            outStr = TextEncodingHelper::decode(QByteArray(tag->value));
        }
    };
    readTag("title", info.title);
    readTag("artist", info.artist);
    readTag("album", info.album);
    AVDictionaryEntry* tagTrack = av_dict_get(formatCtx->metadata, "track", nullptr, 0);
    if (!tagTrack) tagTrack = av_dict_get(formatCtx->metadata, "tracknumber", nullptr, 0);
    if (tagTrack && strlen(tagTrack->value) > 0) {
        info.track = TextEncodingHelper::decode(QByteArray(tagTrack->value));
    }
    readTag("date", info.date);
    if (m_metaCache.tryUpgrade(filePath, fileSize, MetadataPriority::FastProbe, info)) {
        emit metadataReady(filePath, info);
    }
}

void BPBS_Processor::runIoReaderLoop() {
    const size_t IO_BLOCK_SIZE = 4 * 1024 * 1024;
    std::vector<uint8_t> readChunk(IO_BLOCK_SIZE);
    while (!m_stopFlag.load(std::memory_order_relaxed)) {
        if (m_isFastStorage.load(std::memory_order_relaxed)) {
            std::unique_lock<std::mutex> lock(m_ioReaderMutex);
            m_ioReaderCv.wait(lock, [this]() {
                return !m_isFastStorage.load(std::memory_order_relaxed)
                    || m_stopFlag.load(std::memory_order_relaxed);
            });
            if (m_stopFlag.load(std::memory_order_relaxed)) break;
        }
        auto taskOpt = m_demandQueue.claimNextForReader();
        if (!taskOpt.has_value()) {
            std::unique_lock<std::mutex> lock(m_ioReaderMutex);
            m_ioReaderCv.wait_for(lock, std::chrono::milliseconds(50), [this]() {
                return m_stopFlag.load(std::memory_order_relaxed);
            });
            continue;
        }
        BPBS_DemandItem item = std::move(taskOpt.value());
        qint64 fileSize = QFileInfo(item.filePath).size();
        MetadataPriority priority;
        BPBS_AudioMetadata meta;
        if (!m_metaCache.query(item.filePath, fileSize, meta, priority) || priority == MetadataPriority::Unprobed) {
            probeSingleFileMetadata(item.filePath);
            m_metaCache.query(item.filePath, fileSize, meta, priority);
        }
        if (priority == MetadataPriority::InvalidAudio) {
            continue;
        }
        int currentCodecId = meta.codecId;
        auto buffer = std::make_shared<BPBS_Buffer>(32 * 1024 * 1024);
        buffer->setGlobalStopFlag(&m_stopFlag);
        buffer->setFileSize(fileSize);
        m_demandQueue.assignBufferAndCodec(item.filePath, buffer, currentCodecId);
        item.buffer = buffer;
        item.codecId = currentCodecId;
        if (!item.buffer) continue;
        FILE* fp = nullptr;
#ifdef _WIN32
        fp = _wfopen(item.filePath.toStdWString().c_str(), L"rb");
#else
        fp = fopen(item.filePath.toUtf8().constData(), "rb");
#endif
        if (!fp) {
            item.buffer->abort();
            continue;
        }
        while (!m_stopFlag.load(std::memory_order_relaxed)) {
            if (item.buffer->isAborted() || m_stopFlag.load(std::memory_order_relaxed)) {
                break;
            }
            if (m_demandQueue.isMemorySuspended()) {
                item.buffer->abort();
                break;
            }
            int64_t seekTarget = 0;
            if (item.buffer->getSeekRequest(seekTarget)) {
                if (fp) {
#ifdef _WIN32
                    _fseeki64(fp, seekTarget, SEEK_SET);
#else
                    fseeko(fp, seekTarget, SEEK_SET);
#endif
                    clearerr(fp);
                }
                item.buffer->seekCompleted(seekTarget);
                continue;
            }
            if (!item.buffer->isEof() && (item.buffer->capacity() - item.buffer->bytesAvailable()) >= IO_BLOCK_SIZE) {
                size_t bytesRead = fread(readChunk.data(), 1, IO_BLOCK_SIZE, fp);
                if (bytesRead > 0) {
                    item.buffer->write(readChunk.data(), bytesRead);
                }
                if (bytesRead < IO_BLOCK_SIZE && !item.buffer->getSeekRequest(seekTarget)) {
                    if (feof(fp) || ferror(fp)) {
                        item.buffer->setEof();
                    }
                }
            } else {
                QThread::msleep(10);
            }
        }
        if (fp) fclose(fp);
    }
}
#include "BPBS_Worker.h"
#include "BPBS_Decoder.h"
#include "BPBS_FFT.h"
#include "BPBS_Generator.h"
#include "BPBS_Utils.h"
#include "AppConfig.h"

#include <QFileInfo>
#include <QDir>
#include <QThread>
#include <algorithm>
#include <cmath>
#include <cstring>

namespace {
    constexpr size_t READ_CHUNK_SIZE = 32768;
    QString normalizeDirPath(const QString& path) {
        if (path.isEmpty()) return QString();
        QString clean = QDir::cleanPath(path);
        clean.replace('\\', '/');
        return clean;
    }
}

BPBS_Worker::BPBS_Worker(int workerId,
                         int bucketIdPadding,
                         BPBS_DemandQueue& queue,
                         BPBS_Cache& cache,
                         BPBS_MetaCache& metaCache,
                         CalcParamsGetter calcParamsGetter,
                         VisualParamsGetter visualParamsGetter,
                         std::atomic<bool>* stopFlag,
                         std::atomic<bool>* pauseFlag,
                         QObject* parent)
    : QObject(parent)
    , m_workerId(workerId)
    , m_bucketIdPadding(bucketIdPadding)
    , m_queue(queue)
    , m_cache(cache)
    , m_metaCache(metaCache)
    , m_calcParamsGetter(std::move(calcParamsGetter))
    , m_visualParamsGetter(std::move(visualParamsGetter))
    , m_stopFlag(stopFlag)
    , m_pauseFlag(pauseFlag)
{
    setAutoDelete(true);
}

void BPBS_Worker::splitPath(const QString& fullPath, QString& outDomain, QString& outFileName) {
    QString clean = normalizeDirPath(fullPath);
    int lastSlash = clean.lastIndexOf('/');
    if (lastSlash != -1) {
        outDomain = clean.left(lastSlash);
        outFileName = clean.mid(lastSlash + 1);
    } else {
        outDomain.clear();
        outFileName = clean;
    }
}

void BPBS_Worker::run() {
    QThread::currentThread()->setPriority(QThread::LowPriority);
    QElapsedTimer bucketTimer;
    bucketTimer.start();
    BPBS_FFT fft;
    BPBS_Generator generator;
    while (!m_stopFlag->load(std::memory_order_relaxed)) {
        checkPause();
        if (m_stopFlag->load(std::memory_order_relaxed)) break;
        std::shared_ptr<BPBS_CancellationToken> token;
        auto taskOpt = m_queue.claim(m_workerId, token, m_stopFlag, m_pauseFlag);
        if (!taskOpt.has_value()) {
            break;
        }
        BPBS_DemandItem item = std::move(taskOpt.value());
        QString fileName = QFileInfo(item.filePath).fileName();
        emit logMessage(QString(tr("%1 [后台线程 %2] 开始处理：%3"))
            .arg(BPBS_Utils::getCurrentTimestamp())
            .arg(m_workerId, m_bucketIdPadding, 10, QChar('0'))
            .arg(fileName));
        BPBS_CalcParams calcParams = m_calcParamsGetter ? m_calcParamsGetter() : BPBS_CalcParams{};
        BPBS_VisualParams visualParams = m_visualParamsGetter ? m_visualParamsGetter() : BPBS_VisualParams{};
        QString errorMsg;
        qint64 elapsedMs = 0;
        bool success = false;
        try {
            success = processSingleFile(item, calcParams, visualParams, fft, generator, errorMsg, elapsedMs, token);
        } catch (const std::exception& e) {
            errorMsg = tr("异常：%1").arg(e.what());
            success = false;
        } catch (...) {
            errorMsg = tr("发生未知错误");
            success = false;
        }
        if (item.buffer) {
            item.buffer->abort();
            item.buffer.reset();
        }
        QString releasedP1File;
        m_queue.finishTask(m_workerId, &releasedP1File);
        if (!releasedP1File.isEmpty()) {
            emit p1FileReleased(releasedP1File);
        }
        if (success) {
            QString timeStr = BPBS_Utils::formatElapsedTime(elapsedMs);
            QString extraInfo;
            if (errorMsg.contains("WARNING_TRUNCATED")) {
                extraInfo = tr("[音频流存在截断或损坏]");
            }
            emit logMessage(QString(tr("%1 [后台线程 %2] [耗时 %3] 成功缓存%5：%4\n"))
                .arg(BPBS_Utils::getCurrentTimestamp())
                .arg(m_workerId, m_bucketIdPadding, 10, QChar('0'))
                .arg(timeStr)
                .arg(fileName)
                .arg(extraInfo));
        } else {
            if (errorMsg == tr("任务已取消")) {
                emit logMessage(QString(tr("%1 [后台线程 %2] 任务被打断：%3\n"))
                    .arg(BPBS_Utils::getCurrentTimestamp())
                    .arg(m_workerId, m_bucketIdPadding, 10, QChar('0'))
                    .arg(fileName));
            } else {
                emit logMessage(QString(tr("%1 [后台线程 %2] 放弃处理：%3 (原因：%4)\n"))
                    .arg(BPBS_Utils::getCurrentTimestamp())
                    .arg(m_workerId, m_bucketIdPadding, 10, QChar('0'))
                    .arg(fileName)
                    .arg(errorMsg));
            }
        }
    }
    emit bucketFinished(m_workerId, bucketTimer.elapsed());
}

void BPBS_Worker::checkPause() {
    while (m_pauseFlag->load(std::memory_order_relaxed) && !m_stopFlag->load(std::memory_order_relaxed)) {
        QThread::msleep(100);
    }
}

int BPBS_Worker::getRequiredFftSize(int height) const noexcept {
    const auto& opts = AppConfig::FFT_SIZE_OPTIONS;
    for (int size : opts) {
        if (height <= (size / 2 + 1)) return size;
    }
    return opts.back();
}

QImage BPBS_Worker::downsampleIndexed8(const QImage& src, int targetW, int targetH) const {
    if (src.isNull() || src.format() != QImage::Format_Indexed8) return QImage();
    const int srcW = src.width();
    const int srcH = src.height();
    QImage dst(targetW, targetH, QImage::Format_Indexed8);
    dst.setColorTable(src.colorTable());
    for (int y = 0; y < targetH; ++y) {
        int srcY = (y * srcH) / targetH;
        uchar* dstLine = dst.scanLine(y);
        const uchar* srcLine = src.constScanLine(srcY);
        for (int x = 0; x < targetW; ++x) {
            int srcX = (x * srcW) / targetW;
            dstLine[x] = srcLine[srcX];
        }
    }
    return dst;
}

bool BPBS_Worker::processSingleFile(const BPBS_DemandItem& item,
                                    const BPBS_CalcParams& calcParams,
                                    const BPBS_VisualParams& visualParams,
                                    BPBS_FFT& fft,
                                    BPBS_Generator& generator,
                                    QString& errorMsg,
                                    qint64& elapsedMsOut,
                                    std::shared_ptr<BPBS_CancellationToken> token)
{
    QElapsedTimer fileTimer;
    fileTimer.start();
    if (token && token->isCancelled()) {
        errorMsg = tr("任务已取消");
        elapsedMsOut = fileTimer.elapsed();
        return false;
    }
    BPBS_Decoder decoder;
    BPBS_AudioMetadata info;
    bool openSuccess = false;
    if (item.buffer) {
        openSuccess = decoder.open(item.buffer, item.filePath, info, item.codecId, token);
    } else {
        openSuccess = decoder.open(item.filePath, info, token);
    }
    if (!openSuccess) {
        if (token && token->isCancelled()) {
            errorMsg = tr("任务已取消");
            elapsedMsOut = fileTimer.elapsed();
            return false;
        }
        m_metaCache.markInvalidAudio(item.filePath, item.fileSize);
        emit fileNotAudio(item.filePath);
        m_queue.markCompleted(item.filePath);
        errorMsg = tr("解码器打开音频流失败");
        elapsedMsOut = fileTimer.elapsed();
        return false;
    }
    if (info.sampleRateHz <= 0) {
        m_metaCache.markInvalidAudio(item.filePath, item.fileSize);
        emit fileNotAudio(item.filePath);
        m_queue.markCompleted(item.filePath);
        errorMsg = tr("采样率无效");
        elapsedMsOut = fileTimer.elapsed();
        return false;
    }
    if (info.durationSeconds <= 1e-9) {
        if (item.durationMs > 0) {
            info.durationSeconds = static_cast<double>(item.durationMs) / 1000.0;
        } else {
            info.durationSeconds = 300.0;
        }
    }
    int fftSize = getRequiredFftSize(calcParams.imageHeight);
    double effectiveInterval = calcParams.timeInterval;
    bool isAutoPrecision = false;
    if (effectiveInterval <= 1e-9) {
        effectiveInterval = static_cast<double>(fftSize) / info.sampleRateHz;
        isAutoPrecision = true;
    }
    int hopSize = std::max(1, static_cast<int>(info.sampleRateHz * effectiveInterval));
    double overlap = 1.0 - (static_cast<double>(hopSize) / fftSize);
    bool useDouble = (info.sourceBitDepth > 32);
    if (!fft.init(fftSize, overlap, calcParams.windowType, useDouble)) {
        errorMsg = tr("傅里叶变换初始化失败");
        elapsedMsOut = fileTimer.elapsed();
        return false;
    }
    generator.init(calcParams.imageHeight, fftSize, info.sampleRateHz,
                   calcParams.curveType, visualParams.paletteId,
                   visualParams.paletteInverted, visualParams.paletteNegative,
                   calcParams.minDb, calcParams.maxDb);
    long long estWidthLong = (static_cast<long long>(info.durationSeconds * info.sampleRateHz) - fftSize) / hopSize + 1;
    if (estWidthLong <= 0) estWidthLong = 100;
    int estimatedWidth = static_cast<int>(std::min(estWidthLong, 500000LL));
    QImage rawSpectrogram(estimatedWidth, calcParams.imageHeight, QImage::Format_Indexed8);
    if (rawSpectrogram.isNull()) {
        errorMsg = tr("图像渲染内存分配失败");
        elapsedMsOut = fileTimer.elapsed();
        return false;
    }
    rawSpectrogram.fill(0);
    int currentX = 0;
    long long actualTotalSamples = 0;
    while (!m_stopFlag->load(std::memory_order_relaxed)) {
        if (token && token->isCancelled()) {
            errorMsg = tr("任务已取消");
            elapsedMsOut = fileTimer.elapsed();
            return false;
        }
        auto pcmData = decoder.readNextChunk(READ_CHUNK_SIZE);
        size_t currentChunkSize = std::visit([](auto&& arg) { return arg.size(); }, pcmData);
        if (currentChunkSize == 0) break;
        actualTotalSamples += currentChunkSize;
        auto spectrumData = fft.process(pcmData, token);
        if (token && token->isCancelled()) {
            errorMsg = tr("任务已取消");
            elapsedMsOut = fileTimer.elapsed();
            return false;
        }
        QImage strip = generator.generateStrip(spectrumData);
        if (!strip.isNull() && strip.width() > 0) {
            if (rawSpectrogram.colorTable().isEmpty()) {
                rawSpectrogram.setColorTable(strip.colorTable());
            }
            int stripW = strip.width();
            int stripH = strip.height();
            if (currentX + stripW > rawSpectrogram.width()) {
                int growBy = std::max(rawSpectrogram.width() / 2, stripW + 1000);
                growBy = std::min(growBy, 50000);
                int newW = rawSpectrogram.width() + growBy;
                QImage newImg(newW, stripH, QImage::Format_Indexed8);
                if (newImg.isNull()) break;
                newImg.setColorTable(rawSpectrogram.colorTable());
                newImg.fill(0);
                for (int y = 0; y < stripH; ++y) {
                    std::memcpy(newImg.scanLine(y), rawSpectrogram.constScanLine(y), currentX);
                }
                rawSpectrogram = newImg;
            }
            for (int y = 0; y < stripH; ++y) {
                std::memcpy(rawSpectrogram.scanLine(y) + currentX, strip.constScanLine(y), stripW);
            }
            currentX += stripW;
        }
    }
    if (token && token->isCancelled()) {
        errorMsg = tr("任务已取消");
        elapsedMsOut = fileTimer.elapsed();
        return false;
    }
    if (!m_stopFlag->load(std::memory_order_relaxed)) {
        auto residualSpectrum = fft.flush();
        QImage residualStrip = generator.generateStrip(residualSpectrum);
        if (!residualStrip.isNull() && residualStrip.width() > 0) {
            int stripW = residualStrip.width();
            int stripH = residualStrip.height();
            if (currentX + stripW > rawSpectrogram.width()) {
                int newW = rawSpectrogram.width() + std::max(stripW, 100);
                QImage newImg(newW, stripH, QImage::Format_Indexed8);
                if (!newImg.isNull()) {
                    newImg.setColorTable(rawSpectrogram.colorTable());
                    newImg.fill(0);
                    for (int y = 0; y < stripH; ++y) {
                        std::memcpy(newImg.scanLine(y), rawSpectrogram.constScanLine(y), currentX);
                    }
                    rawSpectrogram = newImg;
                }
            }
            if (currentX + stripW <= rawSpectrogram.width()) {
                for (int y = 0; y < stripH; ++y) {
                    std::memcpy(rawSpectrogram.scanLine(y) + currentX, residualStrip.constScanLine(y), stripW);
                }
                currentX += stripW;
            }
        }
    }
    if (m_stopFlag->load(std::memory_order_relaxed) || (token && token->isCancelled())) {
        errorMsg = tr("任务已取消");
        elapsedMsOut = fileTimer.elapsed();
        return false;
    }
    if (currentX == 0) {
        errorMsg = tr("生成的频谱图为空");
        elapsedMsOut = fileTimer.elapsed();
        return false;
    }
    QImage validSpectrogram = rawSpectrogram.copy(0, 0, currentX, calcParams.imageHeight);
    decoder.finalizeExactMetrics();
    info = decoder.getMetadata();
        if (info.sampleRateHz > 0 && actualTotalSamples > 0) {
            info.totalSamples = actualTotalSamples;
            double exactDuration = static_cast<double>(actualTotalSamples) / info.sampleRateHz;
            info.durationSeconds = exactDuration;
            info.preciseDurationSeconds = exactDuration;
            info.durationMicroseconds = static_cast<qint64>(std::llround(exactDuration * 1000000.0));
            info.preciseDurationText = BPBS_Utils::formatSamplesDuration(actualTotalSamples, info.sampleRateHz);
            if (info.totalAudioBytes > 0) {
                auto res = BPBS_Utils::calculatePreciseBitRate(info.totalAudioBytes, actualTotalSamples, info.sampleRateHz);
                if (res.success) {
                    info.bitrateBps = res.bitRateBps;
                    info.preciseBitrateBps = res.bitRateKbpsText.toDouble() * 1000.0;
                    info.preciseBitrateKbpsText = res.bitRateKbpsText;
                }
            }
        }
    if (token && token->isCancelled()) {
        errorMsg = tr("任务已取消");
        elapsedMsOut = fileTimer.elapsed();
        return false;
    }
    QString domain, shortFileName;
    splitPath(item.filePath, domain, shortFileName);
    auto asset = std::make_shared<BPBS_Asset>();
    asset->filePath = item.filePath;
    asset->domainId = domain;
    asset->fileSize = item.fileSize;
    asset->totalAudioBytes = info.totalAudioBytes;
    asset->totalSamples = info.totalSamples;
    asset->fingerprint = calcParams.toFingerprint();
    asset->lastKnownRank = -1;
    asset->calcParams = calcParams;
    asset->metadata = info;
    asset->trackIndex = info.defaultTrackIndex;
    asset->channelIndex = -1;
    asset->l0_image = validSpectrogram;
    asset->l1_icon = downsampleIndexed8(validSpectrogram, 256, 256);
    asset->l2_icon = downsampleIndexed8(validSpectrogram, 96, 96);
    asset->l3_icon = downsampleIndexed8(validSpectrogram, 48, 48);
    asset->l4_icon = downsampleIndexed8(validSpectrogram, 32, 32);
    asset->l5_icon = downsampleIndexed8(validSpectrogram, 16, 16);
    bool shouldSuspend = false;
    bool accepted = m_cache.insert(asset, m_queue.currentB(), shouldSuspend);
    if (shouldSuspend) {
        m_queue.setMemorySuspended(true);
    }
    m_metaCache.tryUpgrade(item.filePath, item.fileSize, MetadataPriority::WorkerDecoded, info);
    m_queue.markCompleted(item.filePath);
    if (accepted) {
        emit thumbnailReady(item.filePath);
    }
    emit metadataReady(item.filePath, info);
    if (decoder.isTruncated()) {
        errorMsg += "WARNING_TRUNCATED";
    }
    elapsedMsOut = fileTimer.elapsed();
    return accepted;
}
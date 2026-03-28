#include "FastSpectrogramProcessor.h"
#include "FastAudioDecoder.h"
#include "FastFft32.h"
#include "FastFft64.h"
#include "FastSpectrogramGenerator.h"
#include "FastUtils.h"

#include <cmath>
#include <algorithm>
#include <QDebug>
#include <QtConcurrent> 
#include <QFuture>

FastSpectrogramProcessor::FastSpectrogramProcessor(QObject* parent):QThread(parent){}

FastSpectrogramProcessor::~FastSpectrogramProcessor() {stopProcessing();wait();}

void FastSpectrogramProcessor::startProcessing(const QString& filePath,
                                               double timeInterval, int imageHeight, int fftSize,
                                               CurveType curveType, double minDb, double maxDb,
                                               PaletteType paletteType, WindowType windowType,
                                               int trackIdx, int channelIdx,
                                               double startSec, double endSec,
                                               std::optional<CueSheet> cueSheet)
{
    if (isRunning()) {
        stopProcessing();
        wait();
    }
    m_filePath = filePath;
    m_timeInterval = timeInterval;
    m_imageHeight = imageHeight;
    m_fftSize = fftSize;
    m_curveType = curveType;
    m_minDb = minDb;
    m_maxDb = maxDb;
    m_paletteType = paletteType;
    m_windowType = windowType;
    m_trackIdx = trackIdx;
    m_channelIdx = channelIdx;
    m_startSec = startSec;
    m_endSec = endSec;
    m_cueSheet = cueSheet;
    m_stopRequested = false;
    start();
}

void FastSpectrogramProcessor::stopProcessing(){m_stopRequested = true;}

template <typename FftEngine, typename PcmType>
double FastSpectrogramProcessor::processLoop(FftEngine& fftEngine, class FastAudioDecoder& decoder) {
    FastSpectrogramGenerator generator;
    connect(&generator, &FastSpectrogramGenerator::logMessage, this, &FastSpectrogramProcessor::logMessage, Qt::DirectConnection);
    int sampleRate = decoder.getMetadata().sampleRate;
    double effectiveInterval = m_timeInterval;
    QString precisionStr;
    if (effectiveInterval <= 0.000000001) {
        if (sampleRate > 0) {
            effectiveInterval = static_cast<double>(m_fftSize) / static_cast<double>(sampleRate);
        } else {
            effectiveInterval = 0.1;
        }
        precisionStr = tr("%1 秒 (自动)").arg(effectiveInterval, 0, 'g', 9);
    } else {
        precisionStr = QString::number(effectiveInterval, 'g', 9) + tr(" 秒");
    }
    int hopSize = std::max(1, static_cast<int>(sampleRate * effectiveInterval + 0.5));
    if (!fftEngine.configure(m_fftSize, hopSize, m_windowType, 0.0)) {
        emit processingFailed(tr("        错误：傅里叶变换初始化失败 内存不足或参数错误"));
        return 0.0;
    }
    emit logMessage(FastUtils::getCurrentTimestamp() + " " + tr("开始傅里叶变换"));
    emit logMessage(tr("        [FFT点数] %1").arg(m_fftSize));
    emit logMessage(tr("        [窗函数] %1").arg(WindowFunctions::getName(m_windowType)));
    emit logMessage(tr("        [时间精度] %1").arg(precisionStr));
    emit logMessage(tr("        [步进]  %1 采样点").arg(hopSize));
    if constexpr (std::is_same_v<PcmType, FastTypes::FastPcmData64>) {
        emit logMessage(tr("        [计算精度] 64 位浮点数"));
    } else {
        emit logMessage(tr("        [计算精度] 32 位浮点数"));
    }
    emit logMessage(FastUtils::getCurrentTimestamp() + " " + tr("开始图像渲染"));
    emit logMessage(tr("        [映射] %1").arg(MappingCurves::getName(m_curveType)));
    emit logMessage(tr("        [配色] %1").arg(ColorPalette::getPaletteNames().value(m_paletteType)));
    emit logMessage(tr("        [ dB ] %1 ~ %2").arg(m_minDb).arg(m_maxDb));
    emit logMessage(FastUtils::getCurrentTimestamp() + " " + tr("循环处理文件"));
    size_t chunkSizeSamples = 32 * hopSize; 
    if (chunkSizeSamples < (size_t)m_fftSize * 2) chunkSizeSamples = m_fftSize * 2;
    long long totalProcessedColumns = 0;
    long long totalSamplesFeed = 0;
    QFuture<FastTypes::FastPcmDataVariant> futurePcm = QtConcurrent::run([&decoder, chunkSizeSamples]() {
        return decoder.readNextChunk(chunkSizeSamples);
    });
    while (!m_stopRequested) {
        auto pcmVariant = futurePcm.result();
        size_t currentChunkSize = std::visit([](auto&& arg){ return arg.size(); }, pcmVariant);
        if (currentChunkSize == 0) break;
        totalSamplesFeed += currentChunkSize;
        futurePcm = QtConcurrent::run([&decoder, chunkSizeSamples]() {
            return decoder.readNextChunk(chunkSizeSamples);
        });
        const PcmType& pcmChunk = std::get<PcmType>(pcmVariant);
        auto spectrumData = fftEngine.process(pcmChunk);
        if (!spectrumData.empty()) {
            QImage imageChunk = generator.generateChunk(
                spectrumData, m_fftSize, m_imageHeight, sampleRate,
                m_curveType, m_minDb, m_maxDb, m_paletteType
            );
            if (!imageChunk.isNull()) {
                double startTime = static_cast<double>(totalProcessedColumns) * hopSize / sampleRate;
                double duration = static_cast<double>(imageChunk.width()) * hopSize / sampleRate;
                emit chunkReady(imageChunk, startTime, duration);
                totalProcessedColumns += imageChunk.width();
            }
        }
    }
    futurePcm.waitForFinished();
    if (!m_stopRequested) {
        auto residualSpectrum = fftEngine.flush();
        if (!residualSpectrum.empty()) {
            QImage imageChunk = generator.generateChunk(
                residualSpectrum, m_fftSize, m_imageHeight, sampleRate,
                m_curveType, m_minDb, m_maxDb, m_paletteType
            );
            if (!imageChunk.isNull()) {
                double startTime = static_cast<double>(totalProcessedColumns) * hopSize / sampleRate;
                double duration = static_cast<double>(imageChunk.width()) * hopSize / sampleRate;
                emit chunkReady(imageChunk, startTime, duration);
                totalProcessedColumns += imageChunk.width();
            }
        }
    }
    if (sampleRate > 0) {
        return static_cast<double>(totalSamplesFeed) / sampleRate;
    }
    return 0.0;
}

void FastSpectrogramProcessor::run(){
    FastAudioDecoder decoder;
    connect(&decoder, &FastAudioDecoder::logMessage, this, &FastSpectrogramProcessor::logMessage, Qt::DirectConnection);
    if (!decoder.open(m_filePath, m_trackIdx, m_channelIdx, m_startSec, m_endSec, m_cueSheet)) {
        emit processingFailed(tr("无法打开音频文件"));
        return;
    }
    auto metadata = decoder.getMetadata();
    emit processingStarted(metadata);
    double pcmDuration = 0.0;
    if (decoder.isDoublePrecision()) {
        FastFft64 fftEngine;
        pcmDuration = processLoop<FastFft64, FastTypes::FastPcmData64>(fftEngine, decoder);
    } else {
        FastFft32 fftEngine;
        pcmDuration = processLoop<FastFft32, FastTypes::FastPcmData32>(fftEngine, decoder);
    }
    if (!m_stopRequested) {
        double finalDuration = pcmDuration;
        if (metadata.durationSec > 0.001) {
            if (pcmDuration > metadata.durationSec + 0.002) {
                long long usec = static_cast<long long>(metadata.durationSec * 1000000.0);
                emit logMessage(tr("        [校准] 检测到填充数据 时长更新为 %1").arg(FastUtils::formatPreciseDuration(usec)));
                finalDuration = metadata.durationSec;
            } 
            else {
                finalDuration = pcmDuration;
            }
        } else {
            finalDuration = pcmDuration;
        }
        emit logMessage(FastUtils::getCurrentTimestamp() + " " + tr("流式处理完成"));
        emit processingFinished(finalDuration);
    } else {
        emit logMessage(FastUtils::getCurrentTimestamp() + " " + tr("任务终止"));
    }
}
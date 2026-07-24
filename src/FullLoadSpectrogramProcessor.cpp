#include "FullLoadSpectrogramProcessor.h"
#include "FullLoadAudioDecoder.h"
#include "AudioDecoderTypes.h"
#include "FFTStrategyFactory.h"
#include "XFullLoadFFTEngine.h"
#include "FullLoadSpectrogramGenerator.h"
#include "AppConfig.h"
#include "FullLoadUtils.h"

#include <variant>
#include <QThread>

FullLoadSpectrogramProcessor::FullLoadSpectrogramProcessor(QObject *parent) : QObject(parent) {}

void FullLoadSpectrogramProcessor::processFile(const QString& filePath,
    double timeInterval, int imageHeight, int fftSize,
    CurveType curveType, double minDb, double maxDb,
    const QString& paletteId, bool paletteInverted, bool paletteNegative,
    FFTWindowType windowType,
    int targetTrackIdx, int targetChannelIdx,
    double startSec, double endSec,
    ProcessMode mode,
    std::optional<CueSheet> cueSheet)
{
    FullLoadAudioDecoder decoder;
    connect(&decoder, &FullLoadAudioDecoder::logMessage, this, &FullLoadSpectrogramProcessor::logMessage);
    if (!decoder.decode(filePath, targetTrackIdx, targetChannelIdx, startSec, endSec, cueSheet)) {
        emit processingFailed(tr("        音频文件解码失败 请检查文件是否有效或损坏"));
        return;
    }
    const PCMDataVariant& pcmData = decoder.getPcmData();
    AudioDecoderTypes::AudioMetadata metadata = decoder.getMetadata();
    emit logMessage(getCurrentTimestamp() + " " + tr("音频解码完成"));
    if (mode == ProcessMode::DecodeOnly) {
        emit processingFinished(QImage(), pcmData, SpectrumDataVariant(), metadata);
        return;
    }
    emit logMessage(getCurrentTimestamp() + " " + tr("开始傅里叶变换"));
    double effectiveInterval = timeInterval;
    QString precisionStr;
    if (effectiveInterval <= 0.000000001) {
        if (metadata.sampleRate > 0) {
            effectiveInterval = static_cast<double>(fftSize) / static_cast<double>(metadata.sampleRate);
        }
        else {
            effectiveInterval = 0.1;
        }
        precisionStr = tr("%1 秒 (自动)").arg(effectiveInterval, 0, 'g', 9);
    }
    else {
        precisionStr = QString::number(effectiveInterval, 'g', 9) + tr(" 秒");
    }
    int hopSize = std::max(1, static_cast<int>(std::round(metadata.sampleRate * effectiveInterval)));
    emit logMessage(tr("        [FFT点数] %1").arg(fftSize));
    emit logMessage(tr("        [窗函数] %1").arg(FFTWindowFunctions::getName(windowType)));
    emit logMessage(tr("        [时间精度] %1").arg(precisionStr));
    emit logMessage(tr("        [步进]  %1 采样点").arg(hopSize));
    size_t totalSamples = std::visit([](auto&& arg) { return arg.size(); }, pcmData);
    FFTParameters params;
    params.fftSize = fftSize;
    params.hopSize = hopSize;
    params.windowType = windowType;
    auto engine = FFTStrategyFactory::create(params, totalSamples, metadata.sourceBitDepth);
    QString precisionBitStr;
    if (engine->getPrecision() == FFTPrecision::Float64) {
        precisionBitStr = tr("64");
    }
    else {
        precisionBitStr = tr("32");
    }
    emit logMessage(tr("        [计算精度] %1 位浮点数").arg(precisionBitStr));
    size_t totalFrames = 0;
    if (totalSamples >= static_cast<size_t>(fftSize)) {
        totalFrames = (totalSamples - fftSize) / hopSize + 1;
    }
    int threadCount = 1;
    if (totalFrames >= static_cast<size_t>(CoreConfig::FFT_MULTITHREAD_THRESHOLD)) {
        threadCount = QThread::idealThreadCount();
        if (threadCount <= 0) threadCount = 2;
    }
    emit logMessage(tr("        [线程] %1 线程").arg(threadCount));

    if (!engine->initialize(params)) {
        emit processingFailed(tr("        傅里叶变换初始化失败 内存不足或参数错误"));
        return;
    }
    auto spectrumResultOpt = engine->compute(pcmData);
    if (!spectrumResultOpt.has_value()) {
        emit processingFailed(tr("傅里叶变换计算失败"));
        return;
    }
    emit logMessage(getCurrentTimestamp() + " " + tr("傅里叶变换完成"));
    emit logMessage(getCurrentTimestamp() + " " + tr("开始图像渲染"));
    emit logMessage(tr("        [映射] %1").arg(MappingCurves::getName(curveType)));
    QString palName = paletteId;
    auto availPals = ColorPaletteFactory::instance().getAvailablePalettes();
    for(const auto& p : availPals) { if(p.first == paletteId) { palName = QString("%1 %2").arg(p.first, p.second); break; } }
    QString palFlags;
    if (paletteInverted) palFlags += tr("(反向)");
    if (paletteNegative) palFlags += tr("(反相)");
    emit logMessage(tr("        [配色] %1 %2").arg(palName).arg(palFlags));
    emit logMessage(tr("        [ dB ] %1 ~ %2").arg(minDb).arg(maxDb));
    FullLoadSpectrogramGenerator generator;
    connect(&generator, &FullLoadSpectrogramGenerator::logMessage, this, &FullLoadSpectrogramProcessor::logMessage);
    QImage resultImage = generator.generate(*spectrumResultOpt, fftSize, imageHeight, metadata.sampleRate, curveType, minDb, maxDb, paletteId, paletteInverted, paletteNegative);
    if (resultImage.isNull()) {
        emit processingFailed(tr("        图像生成失败"));
        return;
    }
    emit logMessage(getCurrentTimestamp() + " " + tr("图像渲染完成"));
    emit processingFinished(resultImage, pcmData, *spectrumResultOpt, metadata);
}

void FullLoadSpectrogramProcessor::reProcessFromPcm(
    const PCMDataVariant& pcmData,
    const AudioDecoderTypes::AudioMetadata& metadata,
    double timeInterval, int imageHeight, int fftSize,
    CurveType curveType, double minDb, double maxDb,
    const QString& paletteId, bool paletteInverted, bool paletteNegative,
    FFTWindowType windowType)
{
    emit logMessage(getCurrentTimestamp() + " " + tr("开始傅里叶变换"));
    double effectiveInterval = timeInterval;
    QString precisionStr;
    if (effectiveInterval <= 0.000000001) {
        if (metadata.sampleRate > 0) {
            effectiveInterval = static_cast<double>(fftSize) / static_cast<double>(metadata.sampleRate);
        }
        else {
            effectiveInterval = 0.1;
        }
        precisionStr = tr("%1 秒 (自动)").arg(effectiveInterval, 0, 'g', 9);
    }
    else {
        precisionStr = QString::number(effectiveInterval, 'g', 9) + tr(" 秒");
    }
    int hopSize = std::max(1, static_cast<int>(metadata.sampleRate * effectiveInterval));
    emit logMessage(tr("        [FFT点数] %1").arg(fftSize));
    emit logMessage(tr("        [窗口] %1").arg(FFTWindowFunctions::getName(windowType)));
    emit logMessage(tr("        [精度] %1").arg(precisionStr));
    emit logMessage(tr("        [步进] %1").arg(hopSize));
    FFTParameters params;
    params.fftSize = fftSize;
    params.hopSize = hopSize;
    params.windowType = windowType;
    size_t totalSamples = std::visit([](auto&& arg) { return arg.size(); }, pcmData);
    auto engine = FFTStrategyFactory::create(params, totalSamples, metadata.sourceBitDepth);
    QString precisionBitStr;
    if (engine->getPrecision() == FFTPrecision::Float64) {
        precisionBitStr = tr("64");
    }
    else {
        precisionBitStr = tr("32");
    }
    emit logMessage(tr("        [计算精度] %1 位").arg(precisionBitStr));
    size_t totalFrames = 0;
    if (totalSamples >= static_cast<size_t>(fftSize)) {
        totalFrames = (totalSamples - fftSize) / hopSize + 1;
    }
    int threadCount = 1;
    if (totalFrames >= static_cast<size_t>(CoreConfig::FFT_MULTITHREAD_THRESHOLD)) {
        threadCount = QThread::idealThreadCount();
        if (threadCount <= 0) threadCount = 2;
    }
    emit logMessage(tr("        [线程] %1 线程").arg(threadCount));
    if (!engine->initialize(params)) {
        emit processingFailed(tr("        傅里叶变换初始化失败"));
        return;
    }
    auto spectrumResultOpt = engine->compute(pcmData);
    if (!spectrumResultOpt.has_value()) {
        emit processingFailed(tr("        傅里叶变换失败"));
        return;
    }
    emit logMessage(getCurrentTimestamp() + " " + tr("傅里叶变换完成"));
    emit logMessage(getCurrentTimestamp() + " " + tr("开始图像渲染"));
    emit logMessage(tr("        [映射] %1").arg(MappingCurves::getName(curveType)));
    QString palName = paletteId;
    auto availPals = ColorPaletteFactory::instance().getAvailablePalettes();
    for(const auto& p : availPals) { if(p.first == paletteId) { palName = QString("%1 %2").arg(p.first, p.second); break; } }
    QString palFlags;
    if (paletteInverted) palFlags += tr("(反向)");
    if (paletteNegative) palFlags += tr("(反相)");
    emit logMessage(tr("        [配色] %1 %2").arg(palName).arg(palFlags));
    emit logMessage(tr("        [ dB ] %1 ~ %2").arg(minDb).arg(maxDb));
    FullLoadSpectrogramGenerator generator;
    connect(&generator, &FullLoadSpectrogramGenerator::logMessage, this, &FullLoadSpectrogramProcessor::logMessage);
    QImage resultImage = generator.generate(*spectrumResultOpt, fftSize, imageHeight, metadata.sampleRate, curveType, minDb, maxDb, paletteId, paletteInverted, paletteNegative);
    if (resultImage.isNull()) {
        emit processingFailed(tr("        图像生成失败"));
        return;
    }
    emit logMessage(getCurrentTimestamp() + " " + tr("图像渲染完成"));
    emit processingFinished(resultImage, {}, *spectrumResultOpt, metadata);
}

void FullLoadSpectrogramProcessor::reProcessFromFft(
    const SpectrumDataVariant& spectrumData,
    const AudioDecoderTypes::AudioMetadata& metadata,
    int fftSize,
    int imageHeight,
    int sampleRate,
    CurveType curveType,
    double minDb,
    double maxDb,
    const QString& paletteId,
    bool paletteInverted,
    bool paletteNegative)
{
    emit logMessage(getCurrentTimestamp() + " " + tr("开始图像渲染"));
    emit logMessage(tr("        [映射] %1").arg(MappingCurves::getName(curveType)));
    QString palName = paletteId;
    auto availPals = ColorPaletteFactory::instance().getAvailablePalettes();
    for(const auto& p : availPals) { if(p.first == paletteId) { palName = QString("%1 %2").arg(p.first, p.second); break; } }
    QString palFlags;
    if (paletteInverted) palFlags += tr("(反向)");
    if (paletteNegative) palFlags += tr("(反相)");
    emit logMessage(tr("        [配色] %1 %2").arg(palName).arg(palFlags));
    emit logMessage(tr("        [ dB ] %1 ~ %2").arg(minDb).arg(maxDb));
    FullLoadSpectrogramGenerator generator;
    connect(&generator, &FullLoadSpectrogramGenerator::logMessage, this, &FullLoadSpectrogramProcessor::logMessage);
    QImage resultImage = generator.generate(spectrumData, fftSize, imageHeight, sampleRate, curveType, minDb, maxDb, paletteId, paletteInverted, paletteNegative);
    if (resultImage.isNull()) {
        emit processingFailed(tr("        图像生成失败"));
        return;
    }
    emit logMessage(getCurrentTimestamp() + " " + tr("图像渲染完成"));
    emit processingFinished(resultImage, {}, spectrumData, metadata);
}
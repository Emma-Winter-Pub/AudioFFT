#include "SpectrogramProcessor.h"
#include "AudioDecoder.h"
#include "FftProcessor.h"
#include "SpectrogramGenerator.h"
#include "AppConfig.h"
#include "Utils.h"


SpectrogramProcessor::SpectrogramProcessor(QObject *parent) : QObject(parent) {}


void SpectrogramProcessor::processFile(
    const QString &filePath,
    double timeInterval,
    int imageHeight,
    int fftSize,
    CurveType curveType)
{
    AudioDecoder decoder;
    connect(&decoder, &AudioDecoder::logMessage, this, &SpectrogramProcessor::logMessage);
    
    if (!decoder.decode(filePath)) {
        emit processingFailed(tr("Decoding failed. Please check if the file is valid or corrupted."));
        return;
    }

    const auto& pcmData = decoder.pcmData();
    const auto& metadata = decoder.metadata();
    
    std::vector<std::vector<double>> spectrogramData;
    FftProcessor fftProcessor;
    connect(&fftProcessor, &FftProcessor::logMessage, this, &SpectrogramProcessor::logMessage);

    if (!fftProcessor.compute(pcmData, spectrogramData, timeInterval, fftSize, metadata.sampleRate)) {
        emit processingFailed(tr("FFT failed. The audio duration might be shorter than the time precision interval."));
        return;
    }
    emit logMessage(getCurrentTimestamp() + tr(" FFT completed"));

    SpectrogramGenerator generator;
    connect(&generator, &SpectrogramGenerator::logMessage, this, &SpectrogramProcessor::logMessage);
    
    QImage resultImage = generator.generate(spectrogramData, imageHeight, curveType);
    if (resultImage.isNull()) {
        emit processingFailed(tr("Image generation failed"));
        return;
    }
    
    double finalDurationSec = metadata.preciseDurationMicroseconds > 0 ?
        static_cast<double>(metadata.preciseDurationMicroseconds) / 1000000.0 :
        metadata.durationSec;

    QString preciseDurationStr = formatPreciseDuration(metadata.preciseDurationMicroseconds);

    emit processingFinished(resultImage, pcmData, finalDurationSec, preciseDurationStr, metadata.sampleRate);
}


void SpectrogramProcessor::reProcessFromPcm(
    const std::vector<float>& pcmData,
    double durationSec,
    const QString& preciseDurationStr,
    int nativeSampleRate, double timeInterval,
    int imageHeight,
    int fftSize,
    CurveType curveType)
{
    emit logMessage(getCurrentTimestamp() + tr(" Recalculation started"));

    std::vector<std::vector<double>> spectrogramData;
    FftProcessor fftProcessor;
    connect(&fftProcessor, &FftProcessor::logMessage, this, &SpectrogramProcessor::logMessage);
    
    if (!fftProcessor.compute(pcmData, spectrogramData, timeInterval, fftSize, nativeSampleRate)) {
        emit processingFailed(tr("FFT recalculation based on cached data failed"));
        return;
    }
    emit logMessage(getCurrentTimestamp() + tr(" FFT completed"));

    SpectrogramGenerator generator;
    connect(&generator, &SpectrogramGenerator::logMessage, this, &SpectrogramProcessor::logMessage);
    
    QImage resultImage = generator.generate(spectrogramData, imageHeight, curveType);
    if (resultImage.isNull()) {
        emit processingFailed(tr("Image generation based on cache failed"));
        return;
    }
    
    emit processingFinished(resultImage, {}, durationSec, preciseDurationStr, nativeSampleRate);
}
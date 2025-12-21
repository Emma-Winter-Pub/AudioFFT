#pragma once

#include "MappingCurves.h"

#include <QObject>
#include <QString>
#include <QImage>
#include <vector>


class SpectrogramProcessor : public QObject {
    Q_OBJECT

public:
    explicit SpectrogramProcessor(QObject *parent = nullptr);

public slots:
    void processFile(
        const QString &filePath,
        double timeInterval,
        int imageHeight,
        int fftSize,
        CurveType curveType);
    void reProcessFromPcm(
        const std::vector<float>& pcmData,
        double durationSec,
        const QString& preciseDurationStr,
        int nativeSampleRate,
        double timeInterval,
        int imageHeight,
        int fftSize,
        CurveType curveType);

signals:
    void logMessage(const QString &message);
    void processingFinished(
        const QImage &spectrogram,
        const std::vector<float>& pcmData,
        double durationSec,
        const QString& preciseDurationStr,
        int nativeSampleRate);
    void processingFailed(const QString &errorMessage);

};
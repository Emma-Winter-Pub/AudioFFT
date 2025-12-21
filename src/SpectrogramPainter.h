#pragma once

#include "MappingCurves.h"

#include <QObject>
#include <QImage>


class SpectrogramPainter : public QObject {
    Q_OBJECT
public:
    explicit SpectrogramPainter(QObject *parent = nullptr);

    QImage drawFinalImage(
        const QImage& rawSpectrogram,
        const QString& fileName,
        double audioDuration,
        bool showGrid,
        const QString& preciseDurationStr,
        int nativeSampleRate,
        CurveType curveType
    );

private:
    QImage createFinalImage_MultiThread(const QImage& rawSpectrogram, const QString& fileName, double audioDuration, bool showGrid, const QString& preciseDurationStr, int nativeSampleRate, CurveType curveType);
    QImage createFinalImage_SingleThread(const QImage& rawSpectrogram, const QString& fileName, double audioDuration, bool showGrid, const QString& preciseDurationStr, int nativeSampleRate, CurveType curveType);

    static int calculateBestFreqStepPng(double maxFreqKhz, int availableHeight);
    static int calculateBestDbStep(double dbRange, int availableHeight);
};
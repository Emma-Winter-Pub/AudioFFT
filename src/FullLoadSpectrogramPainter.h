#pragma once

#include "MappingCurves.h"
#include "ColorPaletteFactory.h"

#include <QObject>
#include <QImage>

class FullLoadSpectrogramPainter : public QObject {
    Q_OBJECT
public:
    explicit FullLoadSpectrogramPainter(QObject *parent = nullptr);
    QImage drawFinalImage(
        const QImage& rawSpectrogram,
        const QString& fileName,
        double audioDuration,
        bool showGrid,
        const QString& preciseDurationStr,
        int nativeSampleRate,
        CurveType curveType,
        double minDb,
        double maxDb,
        const QString& paletteId,
        bool paletteInverted,
        bool paletteNegative,
        bool drawComponents = true 
    );

private:
    QImage createFinalImage_MultiThread(
        const QImage& rawSpectrogram, 
        const QString& fileName, 
        double audioDuration, 
        bool showGrid, 
        const QString& preciseDurationStr, 
        int nativeSampleRate, 
        CurveType curveType, 
        double minDb,
        double maxDb,
        const QString& paletteId,
        bool paletteInverted,
        bool paletteNegative,
        bool drawComponents
    );
    QImage createFinalImage_SingleThread(
        const QImage& rawSpectrogram, 
        const QString& fileName, 
        double audioDuration, 
        bool showGrid, 
        const QString& preciseDurationStr, 
        int nativeSampleRate, 
        CurveType curveType, 
        double minDb,
        double maxDb,
        const QString& paletteId,
        bool paletteInverted,
        bool paletteNegative,
        bool drawComponents
    );
    static int calculateBestFreqStepPng(double maxFreqKhz, int availableHeight);
    static int calculateBestDbStep(double dbRange, int availableHeight);
};
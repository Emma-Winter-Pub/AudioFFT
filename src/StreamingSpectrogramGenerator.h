#pragma once

#include "StreamingTypes.h"
#include "MappingCurves.h"
#include "ColorPaletteFactory.h"

#include <QObject>
#include <QImage>

class StreamingSpectrogramGenerator : public QObject {
    Q_OBJECT

public:
    explicit StreamingSpectrogramGenerator(QObject *parent = nullptr);
    QImage generateChunk(
        const StreamingTypes::StreamingSpectrumDataVariant& spectrumData, 
        int fftSize,
        int targetHeight, 
        int sampleRate,
        CurveType curveType, 
        double minDb, 
        double maxDb, 
        const QString& paletteId,
        bool paletteInverted,
        bool paletteNegative
    );

signals:
    void logMessage(const QString& message);
};
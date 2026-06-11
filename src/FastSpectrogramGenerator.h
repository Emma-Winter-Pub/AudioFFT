#pragma once

#include "FastTypes.h"
#include "MappingCurves.h"
#include "ColorPalette.h"

#include <QObject>
#include <QImage>

class FastSpectrogramGenerator : public QObject {
    Q_OBJECT

public:
    explicit FastSpectrogramGenerator(QObject *parent = nullptr);
    QImage generateChunk(
        const FastTypes::FastSpectrumDataVariant& spectrumData, 
        int fftSize,
        int targetHeight, 
        int sampleRate,
        CurveType curveType, 
        double minDb, 
        double maxDb, 
        PaletteType paletteType
    );

signals:
    void logMessage(const QString& message);
};
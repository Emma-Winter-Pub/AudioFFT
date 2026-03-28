#pragma once

#include "FftTypes.h"
#include "MappingCurves.h"
#include "ColorPalette.h"

#include <QObject>
#include <QImage>

class SpectrogramGenerator : public QObject
{
    Q_OBJECT

public:
    explicit SpectrogramGenerator(QObject *parent = nullptr);
    QImage generate(
        const SpectrumDataVariant& spectrogramData, 
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
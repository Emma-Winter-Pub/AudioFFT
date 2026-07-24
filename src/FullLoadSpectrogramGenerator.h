#pragma once

#include "FFTTypes.h"
#include "MappingCurves.h"
#include "ColorPaletteFactory.h"

#include <QObject>
#include <QImage>

class FullLoadSpectrogramGenerator : public QObject
{
    Q_OBJECT

public:
    explicit FullLoadSpectrogramGenerator(QObject *parent = nullptr);
    QImage generate(
        const SpectrumDataVariant& spectrogramData, 
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
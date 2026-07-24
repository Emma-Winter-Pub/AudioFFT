#pragma once

#include "FFTTypes.h"
#include "MappingCurves.h"
#include "ColorPaletteFactory.h"

#include <QImage>

class BatchFullLoadSpectrogramGenerator
{
public:
    BatchFullLoadSpectrogramGenerator();
    ~BatchFullLoadSpectrogramGenerator();
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
};
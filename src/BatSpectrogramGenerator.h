#pragma once

#include "FftTypes.h"
#include "MappingCurves.h"
#include "ColorPalette.h"

#include <QImage>

class BatSpectrogramGenerator
{
public:
    BatSpectrogramGenerator();
    ~BatSpectrogramGenerator();
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
};
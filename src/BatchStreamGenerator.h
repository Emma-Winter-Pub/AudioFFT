#pragma once

#include "BatchStreamFft.h"
#include "MappingCurves.h"
#include "ColorPalette.h"

#include <QImage>
#include <vector>

class BatchStreamGenerator {
public:
    BatchStreamGenerator();
    ~BatchStreamGenerator();
    void init(int targetHeight, int fftSize, int sampleRate, CurveType curve, PaletteType palette, double minDb, double maxDb);
    QImage generateStrip(const BatchStreamSpectrumVariant& spectrumData);

private:
    template <typename T>
    QImage generateImpl(const std::vector<T, AlignedAllocator<T, 64>>& data);
    int m_height = 0;
    int m_fftSize = 0;
    int m_binCount = 0;
    double m_minDb = -130.0;
    double m_maxDb = 0.0;
    std::vector<int> m_yToBinMap;
    QList<QRgb> m_colorTable;
};
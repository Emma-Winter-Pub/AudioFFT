#include "BatchStreamGenerator.h"
#include "MappingCurves.h"
#include "ColorPalette.h"

#include <cmath>
#include <algorithm>

BatchStreamGenerator::BatchStreamGenerator() {}
BatchStreamGenerator::~BatchStreamGenerator() {}

void BatchStreamGenerator::init(int targetHeight, int fftSize, int sampleRate, CurveType curve, PaletteType palette, double minDb, double maxDb) {
    m_height = targetHeight;
    m_fftSize = fftSize;
    m_binCount = fftSize / 2 + 1;
    m_minDb = minDb;
    m_maxDb = maxDb;
    const double maxFreq = static_cast<double>(sampleRate) / 2.0;
    std::vector<QColor> vecColors = ColorPalette::getPalette(palette);
    if (vecColors.size() < 256) vecColors = ColorPalette::getPalette(PaletteType::S00);
    m_colorTable.clear();
    m_colorTable.reserve(256);
    for (const auto& c : vecColors) m_colorTable.append(c.rgb());
    m_yToBinMap.resize(m_height);
    const double maxBinIdx = (m_binCount > 1) ? (m_binCount - 1) : 0;
    const double invHeight = (m_height > 1) ? 1.0 / (m_height - 1) : 0.0;
    for (int y = 0; y < m_height; ++y) {
        double normalizedY = static_cast<double>(m_height - 1 - y) * invHeight;
        double linearFreq = MappingCurves::inverse(normalizedY, curve, maxFreq);
        if (linearFreq < 0.0) linearFreq = 0.0;
        if (linearFreq > 1.0) linearFreq = 1.0;
        m_yToBinMap[y] = static_cast<int>(std::round(linearFreq * maxBinIdx));
    }
}

QImage BatchStreamGenerator::generateStrip(const BatchStreamSpectrumVariant& spectrumData) {
    auto worker = [&](auto&& arg) { return this->generateImpl(arg); };
    return std::visit(worker, spectrumData);
}

template <typename T>
QImage BatchStreamGenerator::generateImpl(const std::vector<T, AlignedAllocator<T, 64>>& data) {
    if (data.empty() || m_binCount <= 0) return QImage();
    int width = data.size() / m_binCount;
    if (width <= 0) return QImage();
    QImage img(width, m_height, QImage::Format_Indexed8);
    img.setColorTable(m_colorTable);
    const double dbRange = m_maxDb - m_minDb;
    const double invDbRange = (dbRange > 1e-9) ? 1.0 / dbRange : 0.0;
    for (int y = 0; y < m_height; ++y) {
        int binIdx = m_yToBinMap[y];
        uchar* scanLine = img.scanLine(y);
        for (int x = 0; x < width; ++x) {
            size_t dataIdx = static_cast<size_t>(x) * m_binCount + binIdx;
            T val = data[dataIdx];
            double norm = (static_cast<double>(val) - m_minDb) * invDbRange;
            if (norm <= 0.0) scanLine[x] = 0;
            else if (norm >= 1.0) scanLine[x] = 255;
            else scanLine[x] = static_cast<uchar>(norm * 255.0);
        }
    }
    return img;
}
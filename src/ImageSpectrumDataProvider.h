#pragma once

#include "ISpectrumDataProvider.h"

#include <QImage>
#include <vector>

class ImageSpectrumDataProvider : public ISpectrumDataProvider {
public:
    ImageSpectrumDataProvider();
    ~ImageSpectrumDataProvider() override = default;
    void updateImage(const QImage* image);
    bool fetchData(double timePosition, double totalDuration, std::vector<float>& outData, int targetWidth = 0) override;
    double getDbValueAt(double timePosition, double totalDuration, double freqHz, double maxFreq) override;
    int getMaxSignificantDigits() const override { return 0; }

private:
    const QImage* m_image = nullptr;
    qint64 m_lastCacheKey = 0;
    std::vector<uchar> m_columnCache;
    int m_cacheWidth = 0;
    int m_cacheHeight = 0;
    void rebuildCache();
};
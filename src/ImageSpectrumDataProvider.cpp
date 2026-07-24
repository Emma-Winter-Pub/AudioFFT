#include "ImageSpectrumDataProvider.h"

#include <algorithm>
#include <cmath>
#include <cstring>

ImageSpectrumDataProvider::ImageSpectrumDataProvider() {}

void ImageSpectrumDataProvider::updateImage(const QImage* image) {
    m_image = image;
    if (!m_image || m_image->isNull()) {
        m_columnCache.clear();
        m_cacheWidth = 0;
        m_cacheHeight = 0;
        m_lastCacheKey = 0;
        return;
    }
    if (m_image->cacheKey() != m_lastCacheKey) {
        rebuildCache();
        m_lastCacheKey = m_image->cacheKey();
    }
}

void ImageSpectrumDataProvider::rebuildCache() {
    if (!m_image || m_image->format() != QImage::Format_Indexed8) {
        m_columnCache.clear();
        return;
    }
    int w = m_image->width();
    int h = m_image->height();
    size_t totalSize = static_cast<size_t>(w) * h;
    if (m_columnCache.size() != totalSize) {
        m_columnCache.resize(totalSize);
    }
    m_cacheWidth = w;
    m_cacheHeight = h;
    uchar* cacheBase = m_columnCache.data();
    for (int y = 0; y < h; ++y) {
        const uchar* srcLine = m_image->constScanLine(y);
        int destIndexInCol = h - 1 - y;
        for (int x = 0; x < w; ++x) {
            size_t destIdx = static_cast<size_t>(x) * h + destIndexInCol;
            cacheBase[destIdx] = srcLine[x];
        }
    }
}

bool ImageSpectrumDataProvider::fetchData(double timePosition, double totalDuration, std::vector<float>& outData, int targetWidth) {
    if (m_columnCache.empty() || m_cacheWidth <= 0 || m_cacheHeight <= 0 || totalDuration <= 0.000001) {
        return false;
    }
    int col = static_cast<int>((timePosition / totalDuration) * m_cacheWidth);
    if (col < 0) col = 0;
    if (col >= m_cacheWidth) col = m_cacheWidth - 1;
    const uchar* colDataPtr = m_columnCache.data() + (static_cast<size_t>(col) * m_cacheHeight);
    int finalSize = (targetWidth > 0) ? targetWidth : m_cacheHeight;
    if (outData.size() != static_cast<size_t>(finalSize)) {
        outData.resize(finalSize);
    }
    if (finalSize == m_cacheHeight) {
        for (int i = 0; i < finalSize; ++i) {
            outData[i] = static_cast<float>(colDataPtr[i]) / 255.0f;
        }
    }
    else if (finalSize < m_cacheHeight) {
        for (int i = 0; i < finalSize; ++i) {
            int idxStart = (i * m_cacheHeight) / finalSize;
            int idxEnd   = ((i + 1) * m_cacheHeight) / finalSize;
            if (idxEnd <= idxStart) idxEnd = idxStart + 1;
            uchar maxVal = 0;
            for (int k = idxStart; k < idxEnd; ++k) {
                if (colDataPtr[k] > maxVal) {
                    maxVal = colDataPtr[k];
                    if (maxVal == 255) break;
                }
            }
            outData[i] = static_cast<float>(maxVal) / 255.0f;
        }
    }
    else {
        for (int i = 0; i < finalSize; ++i) {
            int srcIdx = static_cast<int>((static_cast<long long>(i) * m_cacheHeight) / finalSize);
            if (srcIdx >= m_cacheHeight) srcIdx = m_cacheHeight - 1;
            outData[i] = static_cast<float>(colDataPtr[srcIdx]) / 255.0f;
        }
    }
    return true;
}

double ImageSpectrumDataProvider::getDbValueAt(double timePosition, double totalDuration, double freqHz, double maxFreq) {
    if (m_columnCache.empty() || totalDuration <= 0.000001 || maxFreq <= 0.000001) {
        return -1000.0;
    }
    int col = static_cast<int>((timePosition / totalDuration) * m_cacheWidth);
    if (col < 0) col = 0;
    if (col >= m_cacheWidth) col = m_cacheWidth - 1;
    double fRatio = freqHz / maxFreq;
    if (fRatio < 0.0) fRatio = 0.0;
    if (fRatio > 1.0) fRatio = 1.0;
    int binIndex = static_cast<int>(std::round(fRatio * (m_cacheHeight - 1)));
    if (binIndex < 0) binIndex = 0;
    if (binIndex >= m_cacheHeight) binIndex = m_cacheHeight - 1;
    const uchar* colDataPtr = m_columnCache.data() + (static_cast<size_t>(col) * m_cacheHeight);
    (void)colDataPtr;
    return -1000.0;
}
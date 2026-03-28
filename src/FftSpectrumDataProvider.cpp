#include "FftSpectrumDataProvider.h"

#include <algorithm>
#include <cmath>
#include <variant>

FftSpectrumDataProvider::FftSpectrumDataProvider(){}

void FftSpectrumDataProvider::setData(const SpectrumDataVariant* data, int fftSize) {
    m_data = data;
    m_fftSize = fftSize;
    if (m_fftSize > 0) {
        m_binCount = m_fftSize / 2 + 1;
    } else {
        m_binCount = 0;
    }
}

void FftSpectrumDataProvider::setDbRange(double minDb, double maxDb) {
    m_minDb = minDb;
    m_maxDb = maxDb;
}

bool FftSpectrumDataProvider::fetchData(double timePosition, double totalDuration, std::vector<float>& outData, int targetWidth) {
    if (!m_data || m_binCount <= 0 || totalDuration <= 0.000001) {
        return false;
    }
    double maxFreq = (m_sampleRate > 0) ? (static_cast<double>(m_sampleRate) / 2.0) : 22050.0;
    auto processData = [&](auto&& vec) -> bool {
        if (vec.empty()) return false;
        size_t totalFrames = vec.size() / m_binCount;
        if (totalFrames == 0) return false;
        double ratio = timePosition / totalDuration;
        if (ratio < 0.0) ratio = 0.0;
        if (ratio >= 1.0) ratio = 0.999999;
        size_t frameIndex = static_cast<size_t>(ratio * totalFrames);
        size_t offset = frameIndex * m_binCount;
        if (offset + m_binCount > vec.size()) return false;
        int finalSize = (targetWidth > 0) ? targetWidth : m_binCount;
        if (outData.size() != static_cast<size_t>(finalSize)) {
            outData.resize(finalSize);
        }
        double dbRange = m_maxDb - m_minDb;
        double invDbRange = (dbRange > 1e-9) ? (1.0 / dbRange) : 0.0;
        const auto* frameData = vec.data() + offset;
        for (int i = 0; i < finalSize; ++i) {
            double y0 = static_cast<double>(i) / finalSize;
            double y1 = static_cast<double>(i + 1) / finalSize;
            double freqRatioStart = MappingCurves::inverse(y0, m_curveType, maxFreq);
            double freqRatioEnd   = MappingCurves::inverse(y1, m_curveType, maxFreq);
            int idxStart = static_cast<int>(freqRatioStart * (m_binCount - 1));
            int idxEnd   = static_cast<int>(freqRatioEnd * (m_binCount - 1));
            if (idxStart < 0) idxStart = 0;
            if (idxEnd >= m_binCount) idxEnd = m_binCount;
            if (idxEnd <= idxStart) idxEnd = idxStart + 1;
            double maxVal = -10000.0; 
            bool first = true;
            for (int k = idxStart; k < idxEnd; ++k) {
                double val = static_cast<double>(frameData[k]);
                if (first || val > maxVal) {
                    maxVal = val;
                    first = false;
                }
            }
            double norm = (maxVal - m_minDb) * invDbRange;
            if (norm < 0.0) norm = 0.0;
            if (norm > 1.0) norm = 1.0;
            outData[i] = static_cast<float>(norm);
        }
        return true;
    };
    return std::visit(processData, *m_data);
}

double FftSpectrumDataProvider::getDbValueAt(double timePosition, double totalDuration, double freqHz, double maxFreq) {
    if (!m_data || m_binCount <= 0 || totalDuration <= 0.000001 || maxFreq <= 0.000001) {
        return -200.0;
    }
    auto queryData = [&](auto&& vec) -> double {
        if (vec.empty()) return -200.0;
        size_t totalFrames = vec.size() / m_binCount;
        if (totalFrames == 0) return -200.0;
        double tRatio = timePosition / totalDuration;
        if (tRatio < 0.0) tRatio = 0.0;
        if (tRatio >= 1.0) tRatio = 0.999999;
        size_t frameIndex = static_cast<size_t>(tRatio * totalFrames);
        size_t offset = frameIndex * m_binCount;
        double fRatio = freqHz / maxFreq;
        if (fRatio < 0.0) fRatio = 0.0;
        if (fRatio > 1.0) fRatio = 1.0;
        int binIndex = static_cast<int>(std::round(fRatio * (m_binCount - 1)));
        if (binIndex < 0) binIndex = 0;
        if (binIndex >= m_binCount) binIndex = m_binCount - 1;
        if (offset + binIndex >= vec.size()) return -200.0;
        return static_cast<double>(vec[offset + binIndex]);
    };
    return std::visit(queryData, *m_data);
}

int FftSpectrumDataProvider::getMaxSignificantDigits() const {
    if (!m_data) return 0;
    if (std::holds_alternative<SpectrumData32>(*m_data)) {
        return 7;
    } else {
        return 15;
    }
}
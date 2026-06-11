#pragma once

#include "ISpectrumDataProvider.h"
#include "FftTypes.h"
#include "MappingCurves.h"

#include <vector>

class FftSpectrumDataProvider : public ISpectrumDataProvider {
public:
    FftSpectrumDataProvider();
    ~FftSpectrumDataProvider() override = default;
    void setData(const SpectrumDataVariant* data, int fftSize);
    void setSampleRate(int sampleRate) { m_sampleRate = sampleRate; }
    void setDbRange(double minDb, double maxDb);
    void setCurveType(CurveType type) { m_curveType = type; }
    bool fetchData(double timePosition, double totalDuration, std::vector<float>& outData, int targetWidth = 0) override;
    double getDbValueAt(double timePosition, double totalDuration, double freqHz, double maxFreq) override;
    int getMaxSignificantDigits() const override;

private:
    const SpectrumDataVariant* m_data = nullptr;
    int m_fftSize = 0;
    int m_binCount = 0;
    int m_sampleRate = 44100;
    double m_minDb = -130.0;
    double m_maxDb = 0.0;
    CurveType m_curveType = CurveType::XX; 
};
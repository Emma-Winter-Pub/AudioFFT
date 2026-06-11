#pragma once

#include <vector>

class ISpectrumDataProvider {
public:
    virtual ~ISpectrumDataProvider() = default;
    virtual bool fetchData(double timePosition, double totalDuration, std::vector<float>& outData, int targetWidth = 0) = 0;
    virtual double getDbValueAt(double timePosition, double totalDuration, double freqHz, double maxFreq) = 0;
    virtual int getMaxSignificantDigits() const { return 15; }
};
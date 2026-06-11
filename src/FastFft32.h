#pragma once

#include "FastTypes.h"
#include "WindowFunctions.h" 
#include "FftwFloatRAII.h"

#include <fftw3.h>
#include <vector>

class FastFft32 {
public:
    FastFft32();
    ~FastFft32();
    bool configure(int fftSize, int hopSize, WindowType windowType, double windowParam = 0.0);
    FastTypes::FastSpectrumData32 process(const FastTypes::FastPcmData32& inputChunk);
    FastTypes::FastSpectrumData32 flush();
    void reset();
    int getBinCount() const { return m_fftSize / 2 + 1; }

private:
    int m_fftSize = 0;
    int m_hopSize = 0;
    int m_binCount = 0;
    size_t m_samplesToSkip = 0;
    FastTypes::FastPcmData32 m_residueBuffer;
    FftwFloat::FloatBufferPtr m_fftInput;
    FftwFloat::ComplexBufferPtr m_fftOutput;
    FftwFloat::PlanPtr m_plan;
    std::vector<float> m_window;
};
#pragma once

#include "FastTypes.h"
#include "WindowFunctions.h"
#include "FftwDoubleRAII.h"

#include <fftw3.h>
#include <vector>

class FastFft64 {
public:
    FastFft64();
    ~FastFft64();
    bool configure(int fftSize, int hopSize, WindowType windowType, double windowParam = 0.0);
    FastTypes::FastSpectrumData64 process(const FastTypes::FastPcmData64& inputChunk);
    FastTypes::FastSpectrumData64 flush();
    void reset();
    int getBinCount() const { return m_fftSize / 2 + 1; }

private:
    int m_fftSize = 0;
    int m_hopSize = 0;
    int m_binCount = 0;
    size_t m_samplesToSkip = 0;
    FastTypes::FastPcmData64 m_residueBuffer;
    FftwDouble::DoubleBufferPtr m_fftInput;
    FftwDouble::ComplexBufferPtr m_fftOutput;
    FftwDouble::PlanPtr m_plan;
    std::vector<double> m_window;
};
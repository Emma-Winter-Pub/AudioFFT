#pragma once

#include "StreamingTypes.h"
#include "FFTWindowFunctions.h"
#include "FFT64RAII.h"

#include <fftw3.h>
#include <vector>

class StreamingFFT64 {
public:
    StreamingFFT64();
    ~StreamingFFT64();
    bool configure(int fftSize, int hopSize, FFTWindowType windowType, double windowParam = 0.0);
    StreamingTypes::StreamingSpectrumData64 process(const StreamingTypes::StreamingPcmData64& inputChunk);
    StreamingTypes::StreamingSpectrumData64 flush();
    void reset();
    int getBinCount() const { return m_fftSize / 2 + 1; }

private:
    int m_fftSize = 0;
    int m_hopSize = 0;
    int m_binCount = 0;
    size_t m_samplesToSkip = 0;
    StreamingTypes::StreamingPcmData64 m_residueBuffer;
    FftwDouble::DoubleBufferPtr m_fftInput;
    FftwDouble::ComplexBufferPtr m_fftOutput;
    FftwDouble::PlanPtr m_plan;
    std::vector<double> m_window;
};
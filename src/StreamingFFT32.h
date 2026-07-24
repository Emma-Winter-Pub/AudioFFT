#pragma once

#include "StreamingTypes.h"
#include "FFTWindowFunctions.h" 
#include "FFT32RAII.h"

#include <fftw3.h>
#include <vector>

class StreamingFFT32 {
public:
    StreamingFFT32();
    ~StreamingFFT32();
    bool configure(int fftSize, int hopSize, FFTWindowType windowType, double windowParam = 0.0);
    StreamingTypes::StreamingSpectrumData32 process(const StreamingTypes::StreamingPcmData32& inputChunk);
    StreamingTypes::StreamingSpectrumData32 flush();
    void reset();
    int getBinCount() const { return m_fftSize / 2 + 1; }

private:
    int m_fftSize = 0;
    int m_hopSize = 0;
    int m_binCount = 0;
    size_t m_samplesToSkip = 0;
    StreamingTypes::StreamingPcmData32 m_residueBuffer;
    FftwFloat::FloatBufferPtr m_fftInput;
    FftwFloat::ComplexBufferPtr m_fftOutput;
    FftwFloat::PlanPtr m_plan;
    std::vector<float> m_window;
};
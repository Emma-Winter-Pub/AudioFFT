#include "StreamingFFT32.h"
#include "FFTWGlobalLock.h"

#include <cmath>
#include <algorithm>
#include <cstring>

StreamingFFT32::StreamingFFT32() {}

StreamingFFT32::~StreamingFFT32() {}

void StreamingFFT32::reset() {
    m_residueBuffer.clear();
    m_samplesToSkip = 0;
    if (m_fftSize > 0) {
        m_residueBuffer.reserve(m_fftSize * 2);
    }
}

bool StreamingFFT32::configure(int fftSize, int hopSize, FFTWindowType windowType, double windowParam) {
    m_plan.reset();
    m_fftInput.reset();
    m_fftOutput.reset();
    reset();
    if (fftSize <= 0 || hopSize <= 0) return false;
    m_fftSize = fftSize;
    m_hopSize = hopSize;
    m_binCount = fftSize / 2 + 1;
    m_fftInput = FftwFloat::allocReal(m_fftSize);
    m_fftOutput = FftwFloat::allocComplex(m_binCount);
    if (!m_fftInput || !m_fftOutput) {
        return false;
    }
    {
        std::lock_guard<std::recursive_mutex> lock(getFFTWGlobalLock());
        m_plan = FftwFloat::PlanPtr(fftwf_plan_dft_r2c_1d(
            m_fftSize, 
            m_fftInput.get(), 
            m_fftOutput.get(), 
            FFTW_ESTIMATE
        ));
    }
    if (!m_plan) {
        return false;
    }
    std::vector<double> winDouble = FFTWindowFunctions::generate(m_fftSize, windowType, windowParam);
    m_window.resize(m_fftSize);
    for (int i = 0; i < m_fftSize; ++i) {
        m_window[i] = static_cast<float>(winDouble[i]);
    }
    return true;
}

StreamingTypes::StreamingSpectrumData32 StreamingFFT32::process(const StreamingTypes::StreamingPcmData32& inputChunk) {
    StreamingTypes::StreamingSpectrumData32 outputSpectrums;
    if (!m_plan) return outputSpectrums;
    if (!inputChunk.empty()) {
        m_residueBuffer.insert(m_residueBuffer.end(), inputChunk.begin(), inputChunk.end());
    }
    size_t processedOffset = m_samplesToSkip;
    float* inPtr = m_fftInput.get();
    fftwf_complex* outPtr = m_fftOutput.get();
    while (processedOffset + m_fftSize <= m_residueBuffer.size()) {
        const float* windowStart = m_residueBuffer.data() + processedOffset;
        for (int i = 0; i < m_fftSize; ++i) {
            inPtr[i] = windowStart[i] * m_window[i];
        }
        fftwf_execute(m_plan.get());
        size_t currentOutputSize = outputSpectrums.size();
        outputSpectrums.resize(currentOutputSize + m_binCount);
        float* outputDest = outputSpectrums.data() + currentOutputSize;
        float normFactorSq = static_cast<float>(m_fftSize) * static_cast<float>(m_fftSize);
        for (int i = 0; i < m_binCount; ++i) {
            float re = outPtr[i][0];
            float im = outPtr[i][1];
            float power = (re * re + im * im) / normFactorSq;
            if (i > 0 && i < m_binCount - 1) power *= 4.0f;
            outputDest[i] = 10.0f * std::log10(power + 1e-18f);
        }
        processedOffset += m_hopSize;
    }
    if (processedOffset < m_residueBuffer.size()) {
        size_t remainingSize = m_residueBuffer.size() - processedOffset;
        std::memmove(m_residueBuffer.data(), m_residueBuffer.data() + processedOffset, remainingSize * sizeof(float));
        m_residueBuffer.resize(remainingSize);
        m_samplesToSkip = 0;
    } 
    else {
        m_samplesToSkip = processedOffset - m_residueBuffer.size();
        m_residueBuffer.clear();
    }
    return outputSpectrums;
}

StreamingTypes::StreamingSpectrumData32 StreamingFFT32::flush() {
    if (m_residueBuffer.empty()) {
        return StreamingTypes::StreamingSpectrumData32();
    }
    if (m_residueBuffer.size() < static_cast<size_t>(m_fftSize)) {
        size_t paddingCount = m_fftSize - m_residueBuffer.size();
        m_residueBuffer.insert(m_residueBuffer.end(), paddingCount, 0.0f);
    }
    return process(StreamingTypes::StreamingPcmData32());
}
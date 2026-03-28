#include "FastFft32.h"

#include <cmath>
#include <algorithm>
#include <cstring>

FastFft32::FastFft32() {}

FastFft32::~FastFft32() {}

void FastFft32::reset() {
    m_residueBuffer.clear();
    m_samplesToSkip = 0;
    if (m_fftSize > 0) {
        m_residueBuffer.reserve(m_fftSize * 2);
    }
}

bool FastFft32::configure(int fftSize, int hopSize, WindowType windowType, double windowParam) {
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
    m_plan = FftwFloat::PlanPtr(fftwf_plan_dft_r2c_1d(
        m_fftSize, 
        m_fftInput.get(), 
        m_fftOutput.get(), 
        FFTW_ESTIMATE
    ));
    if (!m_plan) {
        return false;
    }
    std::vector<double> winDouble = WindowFunctions::generate(m_fftSize, windowType, windowParam);
    m_window.resize(m_fftSize);
    for (int i = 0; i < m_fftSize; ++i) {
        m_window[i] = static_cast<float>(winDouble[i]);
    }
    return true;
}

FastTypes::FastSpectrumData32 FastFft32::process(const FastTypes::FastPcmData32& inputChunk) {
    FastTypes::FastSpectrumData32 outputSpectrums;
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
        float normFactor = static_cast<float>(m_fftSize);
        for (int i = 0; i < m_binCount; ++i) {
            float re = outPtr[i][0];
            float im = outPtr[i][1];
            float mag = std::sqrt(re * re + im * im) / normFactor;
            outputDest[i] = 20.0f * std::log10(mag + 1e-9f);
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

FastTypes::FastSpectrumData32 FastFft32::flush() {
    if (m_residueBuffer.empty()) {
        return FastTypes::FastSpectrumData32();
    }
    if (m_residueBuffer.size() < static_cast<size_t>(m_fftSize)) {
        size_t paddingCount = m_fftSize - m_residueBuffer.size();
        m_residueBuffer.insert(m_residueBuffer.end(), paddingCount, 0.0f);
    }
    return process(FastTypes::FastPcmData32());
}
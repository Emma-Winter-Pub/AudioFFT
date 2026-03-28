#include "FastFft64.h"

#include <cmath>
#include <algorithm>
#include <cstring>

FastFft64::FastFft64() {}

FastFft64::~FastFft64() {}

void FastFft64::reset() {
    m_residueBuffer.clear();
    m_samplesToSkip = 0;
    if (m_fftSize > 0) {
        m_residueBuffer.reserve(m_fftSize * 2);
    }
}

bool FastFft64::configure(int fftSize, int hopSize, WindowType windowType, double windowParam) {
    m_plan.reset();
    m_fftInput.reset();
    m_fftOutput.reset();
    reset();
    if (fftSize <= 0 || hopSize <= 0) return false;
    m_fftSize = fftSize;
    m_hopSize = hopSize;
    m_binCount = fftSize / 2 + 1;
    m_fftInput = FftwDouble::allocReal(m_fftSize);
    m_fftOutput = FftwDouble::allocComplex(m_binCount);
    if (!m_fftInput || !m_fftOutput) {
        return false;
    }
    m_plan = FftwDouble::PlanPtr(fftw_plan_dft_r2c_1d(
        m_fftSize, 
        m_fftInput.get(), 
        m_fftOutput.get(), 
        FFTW_ESTIMATE
    ));
    if (!m_plan) {
        return false;
    }
    m_window = WindowFunctions::generate(m_fftSize, windowType, windowParam);
    return true;
}

FastTypes::FastSpectrumData64 FastFft64::process(const FastTypes::FastPcmData64& inputChunk) {
    FastTypes::FastSpectrumData64 outputSpectrums;
    if (!m_plan) return outputSpectrums;
    if (!inputChunk.empty()) {
        m_residueBuffer.insert(m_residueBuffer.end(), inputChunk.begin(), inputChunk.end());
    }
    size_t processedOffset = m_samplesToSkip;
    double* inPtr = m_fftInput.get();
    fftw_complex* outPtr = m_fftOutput.get();
    while (processedOffset + m_fftSize <= m_residueBuffer.size()) {
        const double* windowStart = m_residueBuffer.data() + processedOffset;
        for (int i = 0; i < m_fftSize; ++i) {
            inPtr[i] = windowStart[i] * m_window[i];
        }
        fftw_execute(m_plan.get());
        size_t currentOutputSize = outputSpectrums.size();
        outputSpectrums.resize(currentOutputSize + m_binCount);
        double* outputDest = outputSpectrums.data() + currentOutputSize;
        double normFactor = static_cast<double>(m_fftSize);
        for (int i = 0; i < m_binCount; ++i) {
            double re = outPtr[i][0];
            double im = outPtr[i][1];
            double mag = std::sqrt(re * re + im * im) / normFactor;
            outputDest[i] = 20.0 * std::log10(mag + 1e-9);
        }
        processedOffset += m_hopSize;
    }
    if (processedOffset < m_residueBuffer.size()) {
        size_t remainingSize = m_residueBuffer.size() - processedOffset;
        std::memmove(m_residueBuffer.data(), m_residueBuffer.data() + processedOffset, remainingSize * sizeof(double));
        m_residueBuffer.resize(remainingSize);
        m_samplesToSkip = 0;
    } 
    else {
        m_samplesToSkip = processedOffset - m_residueBuffer.size();
        m_residueBuffer.clear();
    }
    return outputSpectrums;
}

FastTypes::FastSpectrumData64 FastFft64::flush() {
    if (m_residueBuffer.empty()) {
        return FastTypes::FastSpectrumData64();
    }
    if (m_residueBuffer.size() < static_cast<size_t>(m_fftSize)) {
        size_t paddingCount = m_fftSize - m_residueBuffer.size();
        m_residueBuffer.insert(m_residueBuffer.end(), paddingCount, 0.0);
    }
    return process(FastTypes::FastPcmData64());
}
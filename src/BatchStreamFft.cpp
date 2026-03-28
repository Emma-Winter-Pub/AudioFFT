#include "BatchStreamFft.h"
#include "GlobalFftwLock.h"

#include <cmath>
#include <algorithm>
#include <mutex>
#include <vector>
#include <cstring>
#include <iostream>

BatchStreamFft::BatchStreamFft()
    : m_fftSize(0)
    , m_hopSize(0)
    , m_isDouble(false)
    , m_samplesToSkip(0)
{
}

BatchStreamFft::~BatchStreamFft() {
    std::lock_guard<std::recursive_mutex> lock(getGlobalFftwLock());
    m_planFloat.reset();
    m_inFloat.reset();
    m_outFloat.reset();
    m_planDouble.reset();
    m_inDouble.reset();
    m_outDouble.reset();
}

bool BatchStreamFft::init(int fftSize, double overlapPercent, WindowType windowType, bool precision64) {
    if (fftSize <= 0) return false;
    m_samplesToSkip = 0;
    if (m_fftSize == fftSize &&
        std::abs(m_overlapPercent - overlapPercent) < 1e-5 &&
        m_windowType == windowType &&
        m_isDouble == precision64) {
        if (precision64 && m_planDouble) {
            m_residue64.clear();
            return true;
        }
        if (!precision64 && m_planFloat) {
            m_residue32.clear();
            return true;
        }
    }
    {
        std::lock_guard<std::recursive_mutex> lock(getGlobalFftwLock());
        m_planFloat.reset();
        m_inFloat.reset();
        m_outFloat.reset();
        m_planDouble.reset();
        m_inDouble.reset();
        m_outDouble.reset();
    }
    m_fftSize = fftSize;
    m_overlapPercent = overlapPercent;
    m_windowType = windowType;
    m_isDouble = precision64;
    double hop = static_cast<double>(fftSize) * (1.0 - overlapPercent);
    m_hopSize = std::max(1, static_cast<int>(std::round(hop)));
    m_residue32.clear();
    m_residue32.reserve(fftSize * 2);
    m_residue64.clear();
    m_residue64.reserve(fftSize * 2);
    int N = m_fftSize;
    int binCount = N / 2 + 1;
    m_binCount = binCount;
    if (m_isDouble) {
        m_inDouble = BatchStreamFftRAII::allocRealDouble(N);
        m_outDouble = BatchStreamFftRAII::allocComplexDouble(binCount);
        if (!m_inDouble || !m_outDouble) return false;
        {
            std::lock_guard<std::recursive_mutex> lock(getGlobalFftwLock());
            fftw_plan rawPlan = fftw_plan_dft_r2c_1d(
                N, m_inDouble.get(), m_outDouble.get(), FFTW_ESTIMATE
            );
            m_planDouble.reset(rawPlan);
        }
        if (!m_planDouble) return false;
        m_windowDouble = WindowFunctions::generate(N, windowType);
    } else {
        m_inFloat = BatchStreamFftRAII::allocRealFloat(N);
        m_outFloat = BatchStreamFftRAII::allocComplexFloat(binCount);
        if (!m_inFloat || !m_outFloat) return false;
        {
            std::lock_guard<std::recursive_mutex> lock(getGlobalFftwLock());
            fftwf_plan rawPlan = fftwf_plan_dft_r2c_1d(
                N, m_inFloat.get(), m_outFloat.get(), FFTW_ESTIMATE
            );
            m_planFloat.reset(rawPlan);
        }
        if (!m_planFloat) return false;
        std::vector<double> w = WindowFunctions::generate(N, windowType);
        m_windowFloat.resize(N);
        for(int i=0; i<N; ++i) m_windowFloat[i] = static_cast<float>(w[i]);
    }
    return true;
}

BatchStreamSpectrumVariant BatchStreamFft::process(const BatchStreamPcmVariant& inputPacket) {
    if (std::holds_alternative<BatchStreamPcm32>(inputPacket)) {
        if (m_isDouble) {
            return BatchStreamSpectrum64{};
        }
        return process32(std::get<BatchStreamPcm32>(inputPacket));
    }
    else if (std::holds_alternative<BatchStreamPcm64>(inputPacket)) {
        if (!m_isDouble) {
            return BatchStreamSpectrum32{};
        }
        return process64(std::get<BatchStreamPcm64>(inputPacket));
    }
    return BatchStreamSpectrum32{}; 
}

BatchStreamSpectrumVariant BatchStreamFft::flush() {
    if (m_isDouble) {
        if (m_residue64.empty()) return BatchStreamSpectrum64{};
        if (m_residue64.size() < static_cast<size_t>(m_fftSize)) {
            size_t needed = m_fftSize - m_residue64.size();
            m_residue64.insert(m_residue64.end(), needed, 0.0);
        }
        return process64(BatchStreamPcm64()); 
    } else {
        if (m_residue32.empty()) return BatchStreamSpectrum32{};
        if (m_residue32.size() < static_cast<size_t>(m_fftSize)) {
            size_t needed = m_fftSize - m_residue32.size();
            m_residue32.insert(m_residue32.end(), needed, 0.0f);
        }
        return process32(BatchStreamPcm32());
    }
}

BatchStreamSpectrum32 BatchStreamFft::process32(const BatchStreamPcm32& newData) {
    BatchStreamSpectrum32 results; 
    if (!m_planFloat) return results;
    if (!newData.empty()) {
        if (m_residue32.capacity() < m_residue32.size() + newData.size()) {
            try {
                m_residue32.reserve(m_residue32.size() + newData.size() + m_fftSize); 
            } catch(const std::bad_alloc&) {
                return results;
            }
        }
        m_residue32.insert(m_residue32.end(), newData.begin(), newData.end());
    }
    int N = m_fftSize;
    int binCount = m_binCount;
    float* inPtr = m_inFloat.get();
    fftwf_complex* outPtr = m_outFloat.get();
    const float* winPtr = m_windowFloat.data();
    size_t offset = m_samplesToSkip;
    while (offset + N <= m_residue32.size()) {
        const float* srcPtr = m_residue32.data() + offset;
        for (int i = 0; i < N; ++i) {
            inPtr[i] = srcPtr[i] * winPtr[i];
        }
        fftwf_execute(m_planFloat.get());
        size_t oldSize = results.size();
        results.resize(oldSize + binCount);
        float* resPtr = results.data() + oldSize;
        for (int i = 0; i < binCount; ++i) {
            float re = outPtr[i][0];
            float im = outPtr[i][1];
            float mag = std::sqrt(re * re + im * im);
            resPtr[i] = 20.0f * std::log10(mag / static_cast<float>(N) + 1e-9f);
        }
        offset += m_hopSize;
    }
    if (offset < m_residue32.size()) {
        m_residue32.erase(m_residue32.begin(), m_residue32.begin() + offset);
        m_samplesToSkip = 0;
    } else {
        m_samplesToSkip = offset - m_residue32.size();
        m_residue32.clear();
    }
    return results;
}

BatchStreamSpectrum64 BatchStreamFft::process64(const BatchStreamPcm64& newData) {
    BatchStreamSpectrum64 results;
    if (!m_planDouble) return results;
    if (!newData.empty()) {
        if (m_residue64.capacity() < m_residue64.size() + newData.size()) {
            try {
                m_residue64.reserve(m_residue64.size() + newData.size() + m_fftSize);
            } catch(const std::bad_alloc&) {
                return results;
            }
        }
        m_residue64.insert(m_residue64.end(), newData.begin(), newData.end());
    }
    int N = m_fftSize;
    int binCount = m_binCount;
    double* inPtr = m_inDouble.get();
    fftw_complex* outPtr = m_outDouble.get();
    const double* winPtr = m_windowDouble.data();
    size_t offset = m_samplesToSkip;
    while (offset + N <= m_residue64.size()) {
        const double* srcPtr = m_residue64.data() + offset;
        for (int i = 0; i < N; ++i) {
            inPtr[i] = srcPtr[i] * winPtr[i];
        }
        fftw_execute(m_planDouble.get());
        size_t oldSize = results.size();
        results.resize(oldSize + binCount);
        double* resPtr = results.data() + oldSize;
        for (int i = 0; i < binCount; ++i) {
            double re = outPtr[i][0];
            double im = outPtr[i][1];
            double mag = std::sqrt(re * re + im * im);
            resPtr[i] = 20.0 * std::log10(mag / static_cast<double>(N) + 1e-9);
        }
        offset += m_hopSize;
    }
    if (offset < m_residue64.size()) {
        m_residue64.erase(m_residue64.begin(), m_residue64.begin() + offset);
        m_samplesToSkip = 0;
    } else {
        m_samplesToSkip = offset - m_residue64.size();
        m_residue64.clear();
    }
    return results;
}
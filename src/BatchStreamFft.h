#pragma once

#include "BatchStreamTypes.h"
#include "BatchStreamFftRAII.h" 
#include "WindowFunctions.h"

#include <vector>
#include <variant>

using BatchStreamSpectrum32 = std::vector<float, AlignedAllocator<float, 64>>;
using BatchStreamSpectrum64 = std::vector<double, AlignedAllocator<double, 64>>;
using BatchStreamSpectrumVariant = std::variant<BatchStreamSpectrum32, BatchStreamSpectrum64>;

class BatchStreamFft {
public:
    BatchStreamFft();
    ~BatchStreamFft();
    bool init(int fftSize, double overlapPercent, WindowType windowType, bool precision64);
    BatchStreamSpectrumVariant process(const BatchStreamPcmVariant& inputPcm);
    BatchStreamSpectrumVariant flush();
    int getFftSize() const { return m_fftSize; }
    int getBinCount() const { return m_binCount; }

private:
    BatchStreamSpectrum32 process32(const BatchStreamPcm32& input);
    BatchStreamSpectrum64 process64(const BatchStreamPcm64& input);
    size_t m_samplesToSkip = 0;
    int m_fftSize = 0;
    double m_overlapPercent = -1.0;
    WindowType m_windowType = WindowType::Hann;
    bool m_isDouble = false;
    int m_hopSize = 0;
    int m_binCount = 0;
    BatchStreamFftRAII::FloatBufferPtr m_inFloat;
    BatchStreamFftRAII::FloatComplexBufferPtr m_outFloat;
    BatchStreamFftRAII::FloatPlanPtr m_planFloat;
    std::vector<float> m_windowFloat;
    BatchStreamPcm32 m_residue32;
    BatchStreamFftRAII::DoubleBufferPtr m_inDouble;
    BatchStreamFftRAII::DoubleComplexBufferPtr m_outDouble;
    BatchStreamFftRAII::DoublePlanPtr m_planDouble;
    std::vector<double> m_windowDouble;
    BatchStreamPcm64 m_residue64;
};
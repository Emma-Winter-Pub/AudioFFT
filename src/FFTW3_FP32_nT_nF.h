#pragma once

#include "XFullLoadFFTEngine.h"

#include <memory>

class FFTW3_FP32_nT_nF : public XFullLoadFFTEngine {
public:
    FFTW3_FP32_nT_nF();
    ~FFTW3_FP32_nT_nF() override;
    bool initialize(const FFTParameters& params) override;
    std::optional<SpectrumDataVariant> compute(const PCMDataVariant& inputPcm) override;
    const FFTParameters& getParameters() const override { return m_params; }
    FFTPrecision getPrecision() const override { return FFTPrecision::Float32; }
private:
    struct Impl;
    std::unique_ptr<Impl> m_impl;
    FFTParameters m_params;
};
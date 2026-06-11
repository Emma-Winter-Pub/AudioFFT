#pragma once

#include "IFftEngine.h"

#include <memory>

class FftPipeline32Multi : public IFftEngine {
public:
    FftPipeline32Multi();
    ~FftPipeline32Multi() override;
    bool initialize(const FftParameters& params) override;
    std::optional<SpectrumDataVariant> compute(const PcmDataVariant& inputPcm) override;
    const FftParameters& getParameters() const override { return m_params; }
    FftPrecision getPrecision() const override { return FftPrecision::Float32; }
private:
    struct Impl;
    std::unique_ptr<Impl> m_impl;
    FftParameters m_params;
};
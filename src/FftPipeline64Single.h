#pragma once

#include "IFftEngine.h"

#include <memory>

class FftPipeline64Single : public IFftEngine {
public:
    FftPipeline64Single();
    ~FftPipeline64Single() override;
    bool initialize(const FftParameters& params) override;
    std::optional<SpectrumDataVariant> compute(const PcmDataVariant& inputPcm) override;
    const FftParameters& getParameters() const override { return m_params; }
    FftPrecision getPrecision() const override { return FftPrecision::Float64; }
private:
    struct Impl;
    std::unique_ptr<Impl> m_impl;
    FftParameters m_params;
};
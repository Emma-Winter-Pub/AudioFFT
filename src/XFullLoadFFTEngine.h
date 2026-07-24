#pragma once

#include "FFTTypes.h"

#include <optional>

class XFullLoadFFTEngine {
public:
    virtual ~XFullLoadFFTEngine() = default;
    virtual bool initialize(const FFTParameters& params) = 0;
    virtual std::optional<SpectrumDataVariant> compute(const PCMDataVariant& inputPcm) = 0;
    virtual const FFTParameters& getParameters() const = 0;
    virtual FFTPrecision getPrecision() const = 0;
};
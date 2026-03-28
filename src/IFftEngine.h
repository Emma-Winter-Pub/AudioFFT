#pragma once

#include "FftTypes.h"

#include <optional>

class IFftEngine {
public:
    virtual ~IFftEngine() = default;
    virtual bool initialize(const FftParameters& params) = 0;
    virtual std::optional<SpectrumDataVariant> compute(const PcmDataVariant& inputPcm) = 0;
    virtual const FftParameters& getParameters() const = 0;
    virtual FftPrecision getPrecision() const = 0;
};
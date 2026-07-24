#pragma once

#include "XFullLoadFFTEngine.h"

#include <memory>

class FFTStrategyFactory {
public:
    static std::unique_ptr<XFullLoadFFTEngine> create(
        const FFTParameters& params,
        size_t totalSamples,
        int sourceBitDepth
    );
};
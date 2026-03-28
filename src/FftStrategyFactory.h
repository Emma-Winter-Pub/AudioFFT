#pragma once

#include "IFftEngine.h"

#include <memory>

class FftStrategyFactory {
public:
    static std::unique_ptr<IFftEngine> create(
        const FftParameters& params,
        size_t totalSamples,
        int sourceBitDepth
    );
};
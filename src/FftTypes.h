#pragma once

#include "AlignedAllocator.h"
#include "WindowFunctions.h"

#include <vector>
#include <variant>
#include <string>

using PcmData32 = std::vector<float, AlignedAllocator<float, 64>>;
using PcmData64 = std::vector<double, AlignedAllocator<double, 64>>;
using PcmDataVariant = std::variant<PcmData32, PcmData64>;
using SpectrumData32 = std::vector<float, AlignedAllocator<float, 64>>;
using SpectrumData64 = std::vector<double, AlignedAllocator<double, 64>>;
using SpectrumDataVariant = std::variant<SpectrumData32, SpectrumData64>;

struct FftParameters {
    int fftSize = 4096;
    WindowType windowType = WindowType::Hann;
    double windowParam = 0.0;
    int hopSize = 0;
    int threadCount = 0;
    int batchSize = 0;
};

enum class FftPrecision { Float32, Float64 };
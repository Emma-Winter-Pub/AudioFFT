#pragma once

#include "AlignedAllocator.h"
#include "FFTWindowFunctions.h"

#include <vector>
#include <variant>
#include <string>

using PCMData32 = std::vector<float, AlignedAllocator<float, 64>>;
using PCMData64 = std::vector<double, AlignedAllocator<double, 64>>;
using PCMDataVariant = std::variant<PCMData32, PCMData64>;
using SpectrumData32 = std::vector<float, AlignedAllocator<float, 64>>;
using SpectrumData64 = std::vector<double, AlignedAllocator<double, 64>>;
using SpectrumDataVariant = std::variant<SpectrumData32, SpectrumData64>;

struct FFTParameters {
    int fftSize = 4096;
    FFTWindowType windowType = FFTWindowType::Hann;
    double windowParam = 0.0;
    int hopSize = 0;
    int threadCount = 0;
    int batchSize = 0;
};

enum class FFTPrecision { Float32, Float64 };
#include "AppConfig.h"
#include "FFTStrategyFactory.h"
#include "FFTW3_FP32_1T_nF.h"
#include "FFTW3_FP32_nT_nF.h"
#include "FFTW3_FP64_1T_nF.h"
#include "FFTW3_FP64_nT_nF.h"

#include <algorithm>
#include <cmath>

std::unique_ptr<XFullLoadFFTEngine> FFTStrategyFactory::create(
    const FFTParameters& params,
    size_t totalSamples,
    int sourceBitDepth)
{
    int N = params.fftSize;
    int hop = params.hopSize > 0 ? params.hopSize : (N / 2);
    if (hop < 1) hop = 1;
    size_t totalFrames = 0;
    if (totalSamples >= static_cast<size_t>(N)) {
        totalFrames = (totalSamples - N) / hop + 1;
    }
    bool useMultiThread = (totalFrames >= static_cast<size_t>(CoreConfig::FFT_MULTITHREAD_THRESHOLD));
    bool useDoublePrecision = (sourceBitDepth > 32);
    if (useDoublePrecision) {
        if (useMultiThread) {
            return std::make_unique<FFTW3_FP64_nT_nF>();
        } else {
            return std::make_unique<FFTW3_FP64_1T_nF>();
        }
    } else {
        if (useMultiThread) {
            return std::make_unique<FFTW3_FP32_nT_nF>();
        } else {
            return std::make_unique<FFTW3_FP32_1T_nF>();
        }
    }
}
#include "AppConfig.h"
#include "FftStrategyFactory.h"
#include "FftPipeline32Single.h"
#include "FftPipeline32Multi.h"
#include "FftPipeline64Single.h"
#include "FftPipeline64Multi.h"

#include <algorithm>
#include <cmath>

std::unique_ptr<IFftEngine> FftStrategyFactory::create(
    const FftParameters& params,
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
            return std::make_unique<FftPipeline64Multi>();
        } else {
            return std::make_unique<FftPipeline64Single>();
        }
    } else {
        if (useMultiThread) {
            return std::make_unique<FftPipeline32Multi>();
        } else {
            return std::make_unique<FftPipeline32Single>();
        }
    }
}
#include "FftPipeline32Single.h"
#include "FftwFloatRAII.h"
#include "WindowFunctions.h"

#include <cmath>
#include <algorithm>
#include <vector>

using namespace FftwFloat;

namespace {
    int calculateOptimalBatchSize(int fftSize) {
        if (fftSize >= 65536) return 1;
        if (fftSize >= 16384) return 2;
        if (fftSize >= 8192)  return 4;
        if (fftSize >= 4096)  return 4;
        if (fftSize >= 2048)  return 8;
        if (fftSize >= 1024)  return 8;
        if (fftSize >= 512)   return 16;
        if (fftSize >= 256)   return 32;
        return 128; 
    }
}

struct FftPipeline32Single::Impl {
    FloatBufferPtr inBuffer;
    ComplexBufferPtr outBuffer;
    PlanPtr batchPlan;
    PlanPtr singlePlan;
    std::vector<float> windowData;
    int fftSize = 0;
    int batchSize = 1;
    int binCount = 0;
};

FftPipeline32Single::FftPipeline32Single() : m_impl(std::make_unique<Impl>()) {}

FftPipeline32Single::~FftPipeline32Single() = default;

bool FftPipeline32Single::initialize(const FftParameters& params) {
    m_params = params;
    int N = params.fftSize;
    if (N <= 0) return false;
    m_impl->fftSize = N;
    m_impl->binCount = N / 2 + 1;
    int K = params.batchSize;
    if (K <= 0) K = calculateOptimalBatchSize(N);
    m_impl->batchSize = K;
    size_t totalIn = static_cast<size_t>(N) * K;
    size_t totalOut = static_cast<size_t>(m_impl->binCount) * K;
    m_impl->inBuffer = allocReal(totalIn);
    m_impl->outBuffer = allocComplex(totalOut);
    if (!m_impl->inBuffer || !m_impl->outBuffer) return false;
    int n_rank[] = { N };
    m_impl->batchPlan = PlanPtr(fftwf_plan_many_dft_r2c(
        1, n_rank, K,
        m_impl->inBuffer.get(), NULL, 1, N,
        m_impl->outBuffer.get(), NULL, 1, m_impl->binCount,
        FFTW_ESTIMATE
    ));
    m_impl->singlePlan = PlanPtr(fftwf_plan_dft_r2c_1d(
        N, m_impl->inBuffer.get(), m_impl->outBuffer.get(), FFTW_ESTIMATE
    ));
    if (!m_impl->batchPlan || !m_impl->singlePlan) return false;
    std::vector<double> tempWin = WindowFunctions::generate(N, params.windowType, params.windowParam);
    m_impl->windowData.resize(N);
    for(int i=0; i<N; ++i) {
        m_impl->windowData[i] = static_cast<float>(tempWin[i]);
    }
    return true;
}

std::optional<SpectrumDataVariant> FftPipeline32Single::compute(const PcmDataVariant& inputPcm) {
    const PcmData32* pSrc = std::get_if<PcmData32>(&inputPcm);
    if (!pSrc) return std::nullopt;
    const PcmData32& srcData = *pSrc;
    const int N = m_impl->fftSize;
    const int K = m_impl->batchSize;
    const int binCount = m_impl->binCount;
    int hopSize = m_params.hopSize > 0 ? m_params.hopSize : (N / 2);
    if (hopSize < 1) hopSize = 1;
    size_t totalSamples = srcData.size();
    if (totalSamples == 0) return std::nullopt;
    size_t fullFrames = 0;
    if (totalSamples >= static_cast<size_t>(N)) {
        fullFrames = (totalSamples - N) / hopSize + 1;
    }
    size_t coveredSamples = (fullFrames > 0) ? ((fullFrames - 1) * hopSize + N) : 0;
    bool needPadding = (totalSamples > coveredSamples) && ((fullFrames * hopSize) < totalSamples);
    size_t totalFrames = fullFrames + (needPadding ? 1 : 0);
    SpectrumData32 output;
    try {
        output.resize(totalFrames * binCount);
    } catch (...) {
        return std::nullopt;
    }
    float* inBase = m_impl->inBuffer.get();
    fftwf_complex* outBase = m_impl->outBuffer.get();
    const float* winPtr = m_impl->windowData.data();
    const float* srcPtr = srcData.data();
    float* dstPtr = output.data();
    size_t currentFrameIdx = 0;
    while (currentFrameIdx < fullFrames) {
        int remaining = static_cast<int>(fullFrames - currentFrameIdx);
        bool useBatch = (remaining >= K);
        int framesToProcess = useBatch ? K : 1;
        for (int b = 0; b < framesToProcess; ++b) {
            size_t sampleOffset = (currentFrameIdx + b) * hopSize;
            float* frameIn = inBase + (b * N);
            const float* frameSrc = srcPtr + sampleOffset;
            for (int i = 0; i < N; ++i) {
                frameIn[i] = frameSrc[i] * winPtr[i];
            }
        }
        if (useBatch) fftwf_execute(m_impl->batchPlan.get());
        else fftwf_execute(m_impl->singlePlan.get());
        for (int b = 0; b < framesToProcess; ++b) {
            fftwf_complex* frameOut = outBase + (b * binCount);
            size_t outOffset = (currentFrameIdx + b) * binCount;
            for (int i = 0; i < binCount; ++i) {
                float real = frameOut[i][0];
                float imag = frameOut[i][1];
                float mag = std::sqrt(real * real + imag * imag);
                dstPtr[outOffset + i] = 20.0f * std::log10(mag / static_cast<float>(N) + 1e-9f);
            }
        }
        currentFrameIdx += framesToProcess;
    }
    if (needPadding) {
        size_t sampleOffset = currentFrameIdx * hopSize;
        std::vector<float> padBuffer(N, 0.0f);
        size_t available = totalSamples - sampleOffset;
        if (available > 0) {
            std::memcpy(padBuffer.data(), srcPtr + sampleOffset, available * sizeof(float));
        }
        for (int i = 0; i < N; ++i) {
            inBase[i] = padBuffer[i] * winPtr[i];
        }
        fftwf_execute(m_impl->singlePlan.get());
        size_t outOffset = currentFrameIdx * binCount;
        for (int i = 0; i < binCount; ++i) {
            float real = outBase[i][0];
            float imag = outBase[i][1];
            float mag = std::sqrt(real * real + imag * imag);
            dstPtr[outOffset + i] = 20.0f * std::log10(mag / static_cast<float>(N) + 1e-9f);
        }
    }
    return SpectrumDataVariant(std::move(output));
}
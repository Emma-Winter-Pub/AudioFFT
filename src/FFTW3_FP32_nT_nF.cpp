#include "FFTW3_FP32_nT_nF.h"
#include "FFT32RAII.h"
#include "FFTWindowFunctions.h"
#include "FFTWGlobalLock.h"

#include <cmath>
#include <vector>
#include <thread>
#include <algorithm>

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
    struct ThreadContext {
        FloatBufferPtr inBuffer;
        ComplexBufferPtr outBuffer;
        PlanPtr batchPlan;
        PlanPtr singlePlan;
        bool init(int N, int K, int binCount) {
            size_t totalIn = static_cast<size_t>(N) * K;
            size_t totalOut = static_cast<size_t>(binCount) * K;
            inBuffer = allocReal(totalIn);
            outBuffer = allocComplex(totalOut);
            if(!inBuffer || !outBuffer) return false;
            int n_rank[] = { N };
            {
                std::lock_guard<std::recursive_mutex> lock(getFFTWGlobalLock());
                batchPlan = PlanPtr(fftwf_plan_many_dft_r2c(
                    1, n_rank, K, inBuffer.get(), NULL, 1, N, outBuffer.get(), NULL, 1, binCount, FFTW_ESTIMATE
                ));
                singlePlan = PlanPtr(fftwf_plan_dft_r2c_1d(
                    N, inBuffer.get(), outBuffer.get(), FFTW_ESTIMATE
                ));
            }
            return (batchPlan && singlePlan);
        }
    };
    void workerTask(
        ThreadContext* ctx,
        const float* srcPtr,
        float* dstPtr,
        size_t startFrame,
        size_t endFrame,
        int N, int K, int hopSize, int binCount,
        const float* winPtr
    ) {
        float* inBase = ctx->inBuffer.get();
        fftwf_complex* outBase = ctx->outBuffer.get();
        size_t currentFrameIdx = startFrame;

        while (currentFrameIdx < endFrame) {
            int remaining = static_cast<int>(endFrame - currentFrameIdx);
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
            if (useBatch) fftwf_execute(ctx->batchPlan.get());
            else fftwf_execute(ctx->singlePlan.get());
            for (int b = 0; b < framesToProcess; ++b) {
                fftwf_complex* frameOut = outBase + (b * binCount);
                size_t outOffset = (currentFrameIdx + b) * binCount;
                float normFactorSq = static_cast<float>(N) * static_cast<float>(N);
                for (int i = 0; i < binCount; ++i) {
                    float re = frameOut[i][0];
                    float im = frameOut[i][1];
                    float power = (re * re + im * im) / normFactorSq;
                    if (i > 0 && i < binCount - 1) power *= 4.0f;
                    dstPtr[outOffset + i] = 10.0f * std::log10(power + 1e-18f);
                }
            }
            currentFrameIdx += framesToProcess;
        }
    }
}

struct FFTW3_FP32_nT_nF::Impl {
    std::vector<std::unique_ptr<ThreadContext>> contexts;
    std::vector<float> windowData;
    int fftSize = 0;
    int binCount = 0;
    int batchSize = 1;
    int threadCount = 1;
};

FFTW3_FP32_nT_nF::FFTW3_FP32_nT_nF() : m_impl(std::make_unique<Impl>()) {}

FFTW3_FP32_nT_nF::~FFTW3_FP32_nT_nF() = default;

bool FFTW3_FP32_nT_nF::initialize(const FFTParameters& params) {
    m_params = params;
    int N = params.fftSize;
    if (N <= 0) return false;
    m_impl->fftSize = N;
    m_impl->binCount = N / 2 + 1;
    int K = params.batchSize;
    if (K <= 0) K = calculateOptimalBatchSize(N);
    m_impl->batchSize = K;
    int t = params.threadCount;
    if (t <= 0) t = std::thread::hardware_concurrency();
    if (t < 1) t = 1;
    m_impl->threadCount = t;
    m_impl->contexts.clear();
    for(int i=0; i<t; ++i) {
        auto ctx = std::make_unique<ThreadContext>();
        if (!ctx->init(N, K, m_impl->binCount)) return false;
        m_impl->contexts.push_back(std::move(ctx));
    }
    std::vector<double> tempWin = FFTWindowFunctions::generate(N, params.windowType, params.windowParam);
    m_impl->windowData.resize(N);
    for(int i=0; i<N; ++i) m_impl->windowData[i] = static_cast<float>(tempWin[i]);
    return true;
}

std::optional<SpectrumDataVariant> FFTW3_FP32_nT_nF::compute(const PCMDataVariant& inputPcm) {
    const PCMData32* pSrc = std::get_if<PCMData32>(&inputPcm);
    if (!pSrc) return std::nullopt;
    const PCMData32& srcData = *pSrc;
    const int N = m_impl->fftSize;
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
        output.resize(totalFrames * m_impl->binCount);
    } catch (...) {
        return std::nullopt;
    }
    if (fullFrames > 0) {
        int activeThreads = m_impl->threadCount;
        if (fullFrames < static_cast<size_t>(activeThreads * 4)) activeThreads = 1;
        std::vector<std::thread> threads;
        size_t framesPerThread = fullFrames / activeThreads;
        size_t remainder = fullFrames % activeThreads;
        size_t currentStart = 0;
        for (int i = 0; i < activeThreads; ++i) {
            size_t count = framesPerThread + (i < (int)remainder ? 1 : 0);
            if (count > 0) {
                threads.emplace_back(
                    workerTask,
                    m_impl->contexts[i].get(),
                    srcData.data(),
                    output.data(),
                    currentStart,
                    currentStart + count,
                    N, m_impl->batchSize, hopSize, m_impl->binCount,
                    m_impl->windowData.data()
                );
                currentStart += count;
            }
        }
        for (auto& t : threads) {
            if (t.joinable()) t.join();
        }
    }
    if (needPadding) {
        auto& ctx = m_impl->contexts[0];
        size_t lastFrameIdx = fullFrames;
        size_t sampleOffset = lastFrameIdx * hopSize;
        std::vector<float> padBuffer(N, 0.0f);
        size_t available = totalSamples - sampleOffset;
        if (available > 0) {
            std::memcpy(padBuffer.data(), srcData.data() + sampleOffset, available * sizeof(float));
        }
        float* inPtr = ctx->inBuffer.get();
        fftwf_complex* outPtr = ctx->outBuffer.get();
        const float* winPtr = m_impl->windowData.data();
        for (int i = 0; i < N; ++i) {
            inPtr[i] = padBuffer[i] * winPtr[i];
        }
        fftwf_execute(ctx->singlePlan.get());
        float* dstPtr = output.data() + (lastFrameIdx * m_impl->binCount);
        float normFactorSq = static_cast<float>(N) * static_cast<float>(N);
        for (int i = 0; i < m_impl->binCount; ++i) {
            float re = outPtr[i][0];
            float im = outPtr[i][1];
            float power = (re * re + im * im) / normFactorSq;
            if (i > 0 && i < m_impl->binCount - 1) power *= 4.0f;
            dstPtr[i] = 10.0f * std::log10(power + 1e-18f);
        }
    }
    return SpectrumDataVariant(std::move(output));
}
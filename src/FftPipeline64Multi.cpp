#include "FftPipeline64Multi.h"
#include "FftwDoubleRAII.h"
#include "WindowFunctions.h"

#include <cmath>
#include <vector>
#include <thread>
#include <algorithm>

using namespace FftwDouble;

namespace {
    int calculateOptimalBatchSize(int fftSize) {
        if (fftSize >= 65536) return 1;
        if (fftSize >= 16384) return 2;
        if (fftSize >= 8192)  return 4;
        if (fftSize >= 4096)  return 4;
        if (fftSize >= 2048)  return 8;
        if (fftSize >= 1024)  return 8;
        if (fftSize >= 512)   return 16;
        return 32; 
    }
    struct ThreadContext {
        DoubleBufferPtr inBuffer;
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
            batchPlan = PlanPtr(fftw_plan_many_dft_r2c(
                1, n_rank, K, inBuffer.get(), NULL, 1, N, outBuffer.get(), NULL, 1, binCount, FFTW_ESTIMATE
            ));
            singlePlan = PlanPtr(fftw_plan_dft_r2c_1d(
                N, inBuffer.get(), outBuffer.get(), FFTW_ESTIMATE
            ));
            return (batchPlan && singlePlan);
        }
    };
    void workerTask(
        ThreadContext* ctx,
        const double* srcPtr,
        double* dstPtr,
        size_t startFrame,
        size_t endFrame,
        int N, int K, int hopSize, int binCount,
        const double* winPtr
    ) {
        double* inBase = ctx->inBuffer.get();
        fftw_complex* outBase = ctx->outBuffer.get();
        size_t currentFrameIdx = startFrame;
        while (currentFrameIdx < endFrame) {
            int remaining = static_cast<int>(endFrame - currentFrameIdx);
            bool useBatch = (remaining >= K);
            int framesToProcess = useBatch ? K : 1;
            for (int b = 0; b < framesToProcess; ++b) {
                size_t sampleOffset = (currentFrameIdx + b) * hopSize;
                double* frameIn = inBase + (b * N);
                const double* frameSrc = srcPtr + sampleOffset;
                for (int i = 0; i < N; ++i) {
                    frameIn[i] = frameSrc[i] * winPtr[i];
                }
            }
            if (useBatch) fftw_execute(ctx->batchPlan.get());
            else fftw_execute(ctx->singlePlan.get());
            for (int b = 0; b < framesToProcess; ++b) {
                fftw_complex* frameOut = outBase + (b * binCount);
                size_t outOffset = (currentFrameIdx + b) * binCount;
                for (int i = 0; i < binCount; ++i) {
                    double mag = std::sqrt(frameOut[i][0] * frameOut[i][0] + frameOut[i][1] * frameOut[i][1]);
                    dstPtr[outOffset + i] = 20.0 * std::log10(mag / N + 1e-9);
                }
            }
            currentFrameIdx += framesToProcess;
        }
    }
}

struct FftPipeline64Multi::Impl {
    std::vector<std::unique_ptr<ThreadContext>> contexts;
    std::vector<double> windowData;
    int fftSize = 0;
    int binCount = 0;
    int batchSize = 1;
    int threadCount = 1;
};

FftPipeline64Multi::FftPipeline64Multi() : m_impl(std::make_unique<Impl>()) {}

FftPipeline64Multi::~FftPipeline64Multi() = default;

bool FftPipeline64Multi::initialize(const FftParameters& params) {
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
    m_impl->windowData = WindowFunctions::generate(N, params.windowType, params.windowParam);
    return true;
}

std::optional<SpectrumDataVariant> FftPipeline64Multi::compute(const PcmDataVariant& inputPcm) {
    const PcmData64* pSrc = std::get_if<PcmData64>(&inputPcm);
    if (!pSrc) return std::nullopt;
    const PcmData64& srcData = *pSrc;
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
    SpectrumData64 output;
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
        std::vector<double> padBuffer(N, 0.0);
        size_t available = totalSamples - sampleOffset;
        if (available > 0) {
            std::memcpy(padBuffer.data(), srcData.data() + sampleOffset, available * sizeof(double));
        }
        double* inPtr = ctx->inBuffer.get();
        fftw_complex* outPtr = ctx->outBuffer.get();
        const double* winPtr = m_impl->windowData.data();
        for (int i = 0; i < N; ++i) {
            inPtr[i] = padBuffer[i] * winPtr[i];
        }
        fftw_execute(ctx->singlePlan.get());
        double* dstPtr = output.data() + (lastFrameIdx * m_impl->binCount);
        for (int i = 0; i < m_impl->binCount; ++i) {
            double re = outPtr[i][0];
            double im = outPtr[i][1];
            double mag = std::sqrt(re * re + im * im);
            dstPtr[i] = 20.0 * std::log10(mag / static_cast<double>(N) + 1e-9);
        }
    }
    return SpectrumDataVariant(std::move(output));
}
#include "FftPipeline64Single.h"
#include "FftwDoubleRAII.h"
#include "WindowFunctions.h"

#include <cmath>
#include <algorithm>
#include <vector>

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
}

struct FftPipeline64Single::Impl {
    DoubleBufferPtr inBuffer;
    ComplexBufferPtr outBuffer;
    PlanPtr batchPlan;
    PlanPtr singlePlan;
    std::vector<double> windowData;
    int fftSize = 0;
    int batchSize = 1;
    int binCount = 0;
};

FftPipeline64Single::FftPipeline64Single() : m_impl(std::make_unique<Impl>()) {}

FftPipeline64Single::~FftPipeline64Single() = default;

bool FftPipeline64Single::initialize(const FftParameters& params) {
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
    m_impl->batchPlan = PlanPtr(fftw_plan_many_dft_r2c(
        1, n_rank, K,
        m_impl->inBuffer.get(), NULL, 1, N,
        m_impl->outBuffer.get(), NULL, 1, m_impl->binCount,
        FFTW_ESTIMATE
    ));
    m_impl->singlePlan = PlanPtr(fftw_plan_dft_r2c_1d(
        N, m_impl->inBuffer.get(), m_impl->outBuffer.get(), FFTW_ESTIMATE
    ));
    if (!m_impl->batchPlan || !m_impl->singlePlan) return false;
    m_impl->windowData = WindowFunctions::generate(N, params.windowType, params.windowParam);
    return true;
}

std::optional<SpectrumDataVariant> FftPipeline64Single::compute(const PcmDataVariant& inputPcm) {
    const PcmData64* pSrc = std::get_if<PcmData64>(&inputPcm);
    if (!pSrc) return std::nullopt;
    const PcmData64& srcData = *pSrc;
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
    SpectrumData64 output;
    try {
        output.resize(totalFrames * binCount);
    } catch (...) {
        return std::nullopt;
    }
    double* inBase = m_impl->inBuffer.get();
    fftw_complex* outBase = m_impl->outBuffer.get();
    const double* winPtr = m_impl->windowData.data();
    const double* srcPtr = srcData.data();
    double* dstPtr = output.data();
    size_t currentFrameIdx = 0;
    while (currentFrameIdx < fullFrames) {
        int remaining = static_cast<int>(fullFrames - currentFrameIdx);
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
        if (useBatch) fftw_execute(m_impl->batchPlan.get());
        else fftw_execute(m_impl->singlePlan.get());
        for (int b = 0; b < framesToProcess; ++b) {
            fftw_complex* frameOut = outBase + (b * binCount);
            size_t outOffset = (currentFrameIdx + b) * binCount;
            for (int i = 0; i < binCount; ++i) {
                double real = frameOut[i][0];
                double imag = frameOut[i][1];
                double mag = std::sqrt(real * real + imag * imag);
                dstPtr[outOffset + i] = 20.0 * std::log10(mag / static_cast<double>(N) + 1e-9);
            }
        }
        currentFrameIdx += framesToProcess;
    }
    if (needPadding) {
        size_t sampleOffset = currentFrameIdx * hopSize;
        std::vector<double> padBuffer(N, 0.0);
        size_t available = totalSamples - sampleOffset;
        if (available > 0) {
            std::memcpy(padBuffer.data(), srcPtr + sampleOffset, available * sizeof(double));
        }
        for (int i = 0; i < N; ++i) {
            inBase[i] = padBuffer[i] * winPtr[i];
        }
        fftw_execute(m_impl->singlePlan.get());
        size_t outOffset = currentFrameIdx * binCount;
        for (int i = 0; i < binCount; ++i) {
            double real = outBase[i][0];
            double imag = outBase[i][1];
            double mag = std::sqrt(real * real + imag * imag);
            dstPtr[outOffset + i] = 20.0 * std::log10(mag / static_cast<double>(N) + 1e-9);
        }
    }
    return SpectrumDataVariant(std::move(output));
}
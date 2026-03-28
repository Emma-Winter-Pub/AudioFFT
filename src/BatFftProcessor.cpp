#include "BatFftProcessor.h"
#include "GlobalFftwLock.h"

#include <cmath>
#include <algorithm>
#include <mutex>
#include <cstring>
#include <fftw3.h>

namespace {
    int getOptimalBatchSizeDouble(int fftSize) {
        if (fftSize >= 65536) return 1;
        if (fftSize >= 16384) return 2;
        if (fftSize >= 8192)  return 4;
        if (fftSize >= 4096)  return 4;
        if (fftSize >= 2048)  return 8;
        if (fftSize >= 1024)  return 8;
        if (fftSize >= 512)   return 16;
        return 32; 
    }
    int getOptimalBatchSizeFloat(int fftSize) {
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

BatFftProcessor::BatFftProcessor(QObject *parent) : QObject(parent) {}

BatFftProcessor::~BatFftProcessor() {
    std::lock_guard<std::recursive_mutex> lock(getGlobalFftwLock());
    m_ctxFloat.batchPlan.reset();
    m_ctxFloat.singlePlan.reset();
    m_ctxFloat.inBuffer.reset();
    m_ctxFloat.outBuffer.reset();
    m_ctxDouble.batchPlan.reset();
    m_ctxDouble.singlePlan.reset();
    m_ctxDouble.inBuffer.reset();
    m_ctxDouble.outBuffer.reset();
}

std::optional<SpectrumDataVariant> BatFftProcessor::compute(
    const PcmDataVariant& inputPcm,
    double timeInterval,
    int fftSize,
    int sampleRate,
    WindowType windowType)
{
    if (std::holds_alternative<PcmData32>(inputPcm)) {
        auto res = computeFloat(std::get<PcmData32>(inputPcm), timeInterval, fftSize, sampleRate, windowType);
        if (res) return SpectrumDataVariant(std::move(*res));
    } 
    else if (std::holds_alternative<PcmData64>(inputPcm)) {
        auto res = computeDouble(std::get<PcmData64>(inputPcm), timeInterval, fftSize, sampleRate, windowType);
        if (res) return SpectrumDataVariant(std::move(*res));
    }
    return std::nullopt;
}

std::optional<SpectrumData32> BatFftProcessor::computeFloat(
    const PcmData32& pcmData, double timeInterval, int fftSize, int sampleRate, WindowType windowType)
{
    if (pcmData.empty() || fftSize <= 0 || sampleRate <= 0) return std::nullopt;
    if (!m_ctxFloat.isValid(fftSize, windowType)) {
        m_ctxFloat.cachedFftSize = fftSize;
        m_ctxFloat.cachedWindowType = windowType;
        m_ctxFloat.cachedBatchSize = getOptimalBatchSizeFloat(fftSize);
        int N = fftSize;
        int K = m_ctxFloat.cachedBatchSize;
        int binCount = N / 2 + 1;
        size_t totalIn = static_cast<size_t>(N) * K;
        size_t totalOut = static_cast<size_t>(binCount) * K;
        m_ctxFloat.inBuffer = FftwFloat::allocReal(totalIn);
        m_ctxFloat.outBuffer = FftwFloat::allocComplex(totalOut);
        if (!m_ctxFloat.inBuffer || !m_ctxFloat.outBuffer) return std::nullopt;
        {
            std::lock_guard<std::recursive_mutex> lock(getGlobalFftwLock());
            int n_rank[] = { N };
            m_ctxFloat.batchPlan = FftwFloat::PlanPtr(fftwf_plan_many_dft_r2c(
                1, n_rank, K,
                m_ctxFloat.inBuffer.get(), NULL, 1, N,
                m_ctxFloat.outBuffer.get(), NULL, 1, binCount,
                FFTW_ESTIMATE
            ));
            m_ctxFloat.singlePlan = FftwFloat::PlanPtr(fftwf_plan_dft_r2c_1d(
                N, m_ctxFloat.inBuffer.get(), m_ctxFloat.outBuffer.get(), FFTW_ESTIMATE
            ));
        }
        if (!m_ctxFloat.batchPlan || !m_ctxFloat.singlePlan) return std::nullopt;
        std::vector<double> tempWin = WindowFunctions::generate(N, windowType);
        m_ctxFloat.windowData.resize(N);
        for (int i = 0; i < N; ++i) m_ctxFloat.windowData[i] = static_cast<float>(tempWin[i]);
    }
    const int N = fftSize;
    const int K = m_ctxFloat.cachedBatchSize;
    const int binCount = N / 2 + 1;
    int hopSize = std::max(1, static_cast<int>(sampleRate * timeInterval));
    if (timeInterval <= 0.000000001) {
        hopSize = N / 2;
    }
    size_t totalSamples = pcmData.size();
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
    }
    catch (...) {
        return std::nullopt;
    }
    float* inBase = m_ctxFloat.inBuffer.get();
    fftwf_complex* outBase = m_ctxFloat.outBuffer.get();
    const float* winPtr = m_ctxFloat.windowData.data();
    const float* srcPtr = pcmData.data();
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
        if (useBatch) fftwf_execute(m_ctxFloat.batchPlan.get());
        else fftwf_execute(m_ctxFloat.singlePlan.get());
        for (int b = 0; b < framesToProcess; ++b) {
            fftwf_complex* frameOut = outBase + (b * binCount);
            size_t outOffset = (currentFrameIdx + b) * binCount;
            for (int i = 0; i < binCount; ++i) {
                float mag = std::sqrt(frameOut[i][0] * frameOut[i][0] + frameOut[i][1] * frameOut[i][1]);
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
        fftwf_execute(m_ctxFloat.singlePlan.get());
        size_t outOffset = currentFrameIdx * binCount;
        for (int i = 0; i < binCount; ++i) {
            float mag = std::sqrt(outBase[i][0] * outBase[i][0] + outBase[i][1] * outBase[i][1]);
            dstPtr[outOffset + i] = 20.0f * std::log10(mag / static_cast<float>(N) + 1e-9f);
        }
    }
    return output;
}

std::optional<SpectrumData64> BatFftProcessor::computeDouble(
    const PcmData64& pcmData, double timeInterval, int fftSize, int sampleRate, WindowType windowType)
{
    if (pcmData.empty() || fftSize <= 0 || sampleRate <= 0) return std::nullopt;
    if (!m_ctxDouble.isValid(fftSize, windowType)) {
        m_ctxDouble.cachedFftSize = fftSize;
        m_ctxDouble.cachedWindowType = windowType;
        m_ctxDouble.cachedBatchSize = getOptimalBatchSizeDouble(fftSize);
        int N = fftSize;
        int K = m_ctxDouble.cachedBatchSize;
        int binCount = N / 2 + 1;
        size_t totalIn = static_cast<size_t>(N) * K;
        size_t totalOut = static_cast<size_t>(binCount) * K;
        m_ctxDouble.inBuffer = FftwDouble::allocReal(totalIn);
        m_ctxDouble.outBuffer = FftwDouble::allocComplex(totalOut);
        if (!m_ctxDouble.inBuffer || !m_ctxDouble.outBuffer) return std::nullopt;
        {
            std::lock_guard<std::recursive_mutex> lock(getGlobalFftwLock());
            int n_rank[] = { N };
            m_ctxDouble.batchPlan = FftwDouble::PlanPtr(fftw_plan_many_dft_r2c(
                1, n_rank, K,
                m_ctxDouble.inBuffer.get(), NULL, 1, N,
                m_ctxDouble.outBuffer.get(), NULL, 1, binCount,
                FFTW_ESTIMATE
            ));
            m_ctxDouble.singlePlan = FftwDouble::PlanPtr(fftw_plan_dft_r2c_1d(
                N, m_ctxDouble.inBuffer.get(), m_ctxDouble.outBuffer.get(), FFTW_ESTIMATE
            ));
        }
        if (!m_ctxDouble.batchPlan || !m_ctxDouble.singlePlan) return std::nullopt;
        m_ctxDouble.windowData = WindowFunctions::generate(N, windowType);
    }
    const int N = fftSize;
    const int K = m_ctxDouble.cachedBatchSize;
    const int binCount = N / 2 + 1;
    int hopSize = std::max(1, static_cast<int>(sampleRate * timeInterval));
    if (timeInterval <= 0.000000001) hopSize = N / 2;
    size_t totalSamples = pcmData.size();
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
    }
    catch (...) {
        return std::nullopt;
    }
    double* inBase = m_ctxDouble.inBuffer.get();
    fftw_complex* outBase = m_ctxDouble.outBuffer.get();
    const double* winPtr = m_ctxDouble.windowData.data();
    const double* srcPtr = pcmData.data();
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
        if (useBatch) fftw_execute(m_ctxDouble.batchPlan.get());
        else fftw_execute(m_ctxDouble.singlePlan.get());
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
        fftw_execute(m_ctxDouble.singlePlan.get());
        size_t outOffset = currentFrameIdx * binCount;
        for (int i = 0; i < binCount; ++i) {
            double mag = std::sqrt(outBase[i][0] * outBase[i][0] + outBase[i][1] * outBase[i][1]);
            dstPtr[outOffset + i] = 20.0 * std::log10(mag / N + 1e-9);
        }
    }
    return output;
}
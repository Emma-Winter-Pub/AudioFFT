#pragma once

#include "FFTTypes.h"
#include "FFT32RAII.h"
#include "FFT64RAII.h"
#include "FFTWindowFunctions.h"

#include <QObject>
#include <vector>
#include <optional>
#include <variant>

class BatchFullLoadFFTProcessor : public QObject {
    Q_OBJECT

public:
    explicit BatchFullLoadFFTProcessor(QObject *parent = nullptr);
    ~BatchFullLoadFFTProcessor();
    std::optional<SpectrumDataVariant> compute(
        const PCMDataVariant& inputPcm,
        double timeInterval,
        int fftSize,
        int sampleRate,
        FFTWindowType windowType
    );

private:
    struct FloatContext {
        int cachedFftSize = 0;
        int cachedBatchSize = 0;
        FFTWindowType cachedWindowType = FFTWindowType::Rectangular;
        FftwFloat::FloatBufferPtr inBuffer;
        FftwFloat::ComplexBufferPtr outBuffer;
        FftwFloat::PlanPtr batchPlan;
        FftwFloat::PlanPtr singlePlan;
        std::vector<float> windowData;
        bool isValid(int fftSize, FFTWindowType winType) const {
            return (fftSize == cachedFftSize && 
                    cachedWindowType == winType && 
                    inBuffer && outBuffer && batchPlan && singlePlan);
        }
    };
    struct DoubleContext {
        int cachedFftSize = 0;
        int cachedBatchSize = 0;
        FFTWindowType cachedWindowType = FFTWindowType::Rectangular;
        FftwDouble::DoubleBufferPtr inBuffer;
        FftwDouble::ComplexBufferPtr outBuffer;
        FftwDouble::PlanPtr batchPlan;
        FftwDouble::PlanPtr singlePlan;
        std::vector<double> windowData;
        bool isValid(int fftSize, FFTWindowType winType) const {
            return (fftSize == cachedFftSize && 
                    cachedWindowType == winType && 
                    inBuffer && outBuffer && batchPlan && singlePlan);
        }
    };
    FloatContext m_ctxFloat;
    DoubleContext m_ctxDouble;
    std::optional<SpectrumData32> computeFloat(
        const PCMData32& pcm, double timeInterval, int fftSize, int sampleRate, FFTWindowType windowType
    );
    std::optional<SpectrumData64> computeDouble(
        const PCMData64& pcm, double timeInterval, int fftSize, int sampleRate, FFTWindowType windowType
    );
};
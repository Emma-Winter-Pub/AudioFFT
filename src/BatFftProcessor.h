#pragma once

#include "FftTypes.h"
#include "FftwFloatRAII.h"
#include "FftwDoubleRAII.h"
#include "WindowFunctions.h"

#include <QObject>
#include <vector>
#include <optional>
#include <variant>

class BatFftProcessor : public QObject {
    Q_OBJECT

public:
    explicit BatFftProcessor(QObject *parent = nullptr);
    ~BatFftProcessor();
    std::optional<SpectrumDataVariant> compute(
        const PcmDataVariant& inputPcm,
        double timeInterval,
        int fftSize,
        int sampleRate,
        WindowType windowType
    );

private:
    struct FloatContext {
        int cachedFftSize = 0;
        int cachedBatchSize = 0;
        WindowType cachedWindowType = WindowType::Rectangular;
        FftwFloat::FloatBufferPtr inBuffer;
        FftwFloat::ComplexBufferPtr outBuffer;
        FftwFloat::PlanPtr batchPlan;
        FftwFloat::PlanPtr singlePlan;
        std::vector<float> windowData;
        bool isValid(int fftSize, WindowType winType) const {
            return (fftSize == cachedFftSize && 
                    cachedWindowType == winType && 
                    inBuffer && outBuffer && batchPlan && singlePlan);
        }
    };
    struct DoubleContext {
        int cachedFftSize = 0;
        int cachedBatchSize = 0;
        WindowType cachedWindowType = WindowType::Rectangular;
        FftwDouble::DoubleBufferPtr inBuffer;
        FftwDouble::ComplexBufferPtr outBuffer;
        FftwDouble::PlanPtr batchPlan;
        FftwDouble::PlanPtr singlePlan;
        std::vector<double> windowData;
        bool isValid(int fftSize, WindowType winType) const {
            return (fftSize == cachedFftSize && 
                    cachedWindowType == winType && 
                    inBuffer && outBuffer && batchPlan && singlePlan);
        }
    };
    FloatContext m_ctxFloat;
    DoubleContext m_ctxDouble;
    std::optional<SpectrumData32> computeFloat(
        const PcmData32& pcm, double timeInterval, int fftSize, int sampleRate, WindowType windowType
    );
    std::optional<SpectrumData64> computeDouble(
        const PcmData64& pcm, double timeInterval, int fftSize, int sampleRate, WindowType windowType
    );
};
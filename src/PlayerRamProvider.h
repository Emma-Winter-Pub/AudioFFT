#pragma once

#include "IPlayerProvider.h"
#include "FftTypes.h"
#include "PlayerResampler.h"

#include <mutex>

class PlayerRamProvider : public IPlayerProvider {
public:
    PlayerRamProvider(const PcmDataVariant& pcmData, int sampleRate);
    qint64 readSamples(float* buffer, qint64 maxSamples) override;
    void seek(double seconds) override;
    double currentTime() const override;
    bool atEnd() const override;
    int getNativeSampleRate() const override { return m_sampleRate; }
    void configureResampler(int targetRate) override;
    double getLastDecodedTime() const override;
    double getInternalBufferDuration() const override;

private:
    const PcmDataVariant& m_pcmData;
    int m_sampleRate;
    size_t m_currentPos = 0;
    PlayerResampler m_resampler;
    std::vector<float> m_resampledCache;
    size_t m_cachePos = 0;
    mutable std::mutex m_mutex;
    void fillCache();
    int m_targetRate = 44100;
};
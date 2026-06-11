#pragma once

#include <vector>
#include <memory>
#include <cstdint>

extern "C" {
#include <libswresample/swresample.h>
#include <libavutil/samplefmt.h>
#include <libavutil/channel_layout.h>
}

class PlayerResampler {
public:
    PlayerResampler();
    ~PlayerResampler();
    bool init(int srcSampleRate, int srcChannels, AVSampleFormat srcFmt, int targetRate, int targetChannelIdx = -1);
    std::vector<float> process(const void* srcData, int srcNbSamples);
    std::vector<float> process(const uint8_t** srcData, int srcNbSamples);
    void reset();
    int getOutputChannels() const { return m_targetChannels; }
    int getOutputSampleRate() const { return m_targetRate; }

private:
    SwrContext* m_swrCtx = nullptr;
    int m_srcRate = 0;
    int m_targetRate = 44100;
    const int m_targetChannels = 2;
    std::vector<float> processInternal(const uint8_t** inputData, int srcNbSamples);
};
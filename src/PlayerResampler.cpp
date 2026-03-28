#include "PlayerResampler.h"

PlayerResampler::PlayerResampler() {}

PlayerResampler::~PlayerResampler() {if (m_swrCtx) swr_free(&m_swrCtx);}

bool PlayerResampler::init(int srcSampleRate, int srcChannels, AVSampleFormat srcFmt, int targetRate, int targetChannelIdx) {
    if (m_swrCtx) swr_free(&m_swrCtx);
    m_srcRate = srcSampleRate;
    m_targetRate = targetRate;
    AVChannelLayout inLayout, outLayout;
    av_channel_layout_default(&inLayout, srcChannels);
    av_channel_layout_default(&outLayout, m_targetChannels);
    AVSampleFormat inFmt = srcFmt;
    AVSampleFormat outFmt = AV_SAMPLE_FMT_FLT; 
    swr_alloc_set_opts2(&m_swrCtx, 
                        &outLayout, outFmt, m_targetRate,
                        &inLayout, inFmt, srcSampleRate,
                        0, nullptr);
    if (!m_swrCtx) return false;
    if (targetChannelIdx >= 0 && targetChannelIdx < srcChannels) {
        std::vector<double> matrix(m_targetChannels * srcChannels, 0.0);
        matrix[0 * srcChannels + targetChannelIdx] = 1.0;
        matrix[1 * srcChannels + targetChannelIdx] = 1.0;
        if (swr_set_matrix(m_swrCtx, matrix.data(), 0) < 0) {
        }
    }
    if (swr_init(m_swrCtx) < 0) {
        return false;
    }
    return true;
}

std::vector<float> PlayerResampler::processInternal(const uint8_t** inputData, int srcNbSamples) {
    if (!m_swrCtx) return {};
    int outNbSamplesPerChannel = av_rescale_rnd(swr_get_delay(m_swrCtx, m_srcRate) + srcNbSamples, 
                                                m_targetRate, m_srcRate, AV_ROUND_UP);
    std::vector<float> outBuffer(outNbSamplesPerChannel * m_targetChannels + 32);
    uint8_t* outPtr = reinterpret_cast<uint8_t*>(outBuffer.data());
    int convertedSamplesPerChannel = swr_convert(m_swrCtx, &outPtr, outNbSamplesPerChannel, inputData, srcNbSamples);
    if (convertedSamplesPerChannel > 0) {
        outBuffer.resize(convertedSamplesPerChannel * m_targetChannels);
        return outBuffer;
    }
    return {};
}

std::vector<float> PlayerResampler::process(const void* srcData, int srcNbSamples) {
    const uint8_t* inPtr = reinterpret_cast<const uint8_t*>(srcData);
    const uint8_t* inData[1] = { inPtr };
    return processInternal(inData, srcNbSamples);
}

std::vector<float> PlayerResampler::process(const uint8_t** srcData, int srcNbSamples) {
    return processInternal(srcData, srcNbSamples);
}

void PlayerResampler::reset() {if (m_swrCtx) {}}
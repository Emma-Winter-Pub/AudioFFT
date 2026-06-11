#pragma once

#include "IPlayerProvider.h"
#include "PlayerResampler.h"
#include "DecoderTypes.h"

#include <QString>
#include <vector>
#include <mutex>
#include <memory>

class IOBufferStation;

class PlayerFfmpegProvider : public IPlayerProvider {
public:
    PlayerFfmpegProvider(const QString& filePath, int trackIndex, int targetChannelIdx = -1, 
                         double startSec = 0.0, double endSec = 0.0, 
                         size_t bufferCapacity = 16 * 1024 * 1024);
    ~PlayerFfmpegProvider();
    qint64 readSamples(float* buffer, qint64 maxFloatCount) override;
    void seek(double seconds) override;
    double currentTime() const override;
    bool atEnd() const override;
    bool isValid() const { return m_isValid; }
    int getNativeSampleRate() const override;
    void configureResampler(int targetRate) override;
    double getLastDecodedTime() const override;
    double getInternalBufferDuration() const override;

private:
    DecoderTypes::AVFormatContextPtr m_formatCtx;
    DecoderTypes::AVCodecContextPtr m_codecCtx;
    DecoderTypes::AVPacketPtr m_packet;
    DecoderTypes::AVFramePtr m_frame;
    PlayerResampler m_resampler;
    std::vector<float> m_sampleBuffer;
    size_t m_bufferOffset = 0;
    int m_streamIndex = -1;
    int m_targetChannelIdx = -1;
    double m_timeBase = 0.0;
    double m_duration = 0.0;
    bool m_isValid = false;
    bool m_isEof = false;
    double m_currentTimeSec = 0.0;
    double m_lastDecodedPtsEnd = 0.0; 
    int m_targetRate = 44100;
    mutable std::mutex m_mutex;
    bool decodeNextFrame();
    double m_startSec = 0.0;
    double m_endSec = 0.0;
    bool m_isRangeMode = false;
    bool m_isFirstFrameAfterSeek = false;
    std::unique_ptr<IOBufferStation> m_ioStation;
    unsigned char* m_avioBuffer = nullptr;
    AVIOContext* m_avioCtx = nullptr;
    bool m_isReadEof = false;
};
#pragma once

#include "FastTypes.h"
#include "CueParser.h"

#include <QObject>
#include <mutex>
#include <optional> 

struct AVFormatContext;

class FastAudioDecoder : public QObject{
    Q_OBJECT

public:
    explicit FastAudioDecoder(QObject* parent = nullptr);
    ~FastAudioDecoder();
    bool open(const QString& filePath, int targetTrackIdx = -1, int targetChannelIdx = -1, 
              double startSec = 0.0, double endSec = 0.0, 
              std::optional<CueSheet> cueSheet = std::nullopt);
    FastTypes::FastPcmDataVariant readNextChunk(size_t minSamples = 4096);
    bool seek(double timestampSeconds);
    void close();
    const FastTypes::FastAudioMetadata& getMetadata() const { return m_metadata; }
    bool isDoublePrecision() const { return m_useDoublePrecision; }

signals:
    void logMessage(const QString& msg);

private:
    void resetState();
    bool buildChannelMatrix(SwrContext* swr, int inputChannels, int targetChannelIdx);
    static QString getChannelLayoutString(const AVChannelLayout* ch_layout);
    void logFileDetails(struct AVFormatContext* formatContext, double totalDuration);
    FastTypes::AVFormatContextPtr m_formatCtx;
    FastTypes::AVCodecContextPtr  m_codecCtx;
    FastTypes::SwrContextPtr      m_swrCtx;
    FastTypes::AVPacketPtr m_packet;
    FastTypes::AVFramePtr  m_frame;
    FastTypes::FastAudioMetadata m_metadata;
    int m_streamIndex = -1;
    bool m_useDoublePrecision = false;
    bool m_eofReached = false;
    std::recursive_mutex m_mutex; 
    double m_startSec = 0.0;
    double m_endSec = 0.0;
    bool m_isRangeMode = false;
    double m_nextExpectedTime = 0.0; 
    double m_timeBase = 0.0;         
    std::optional<CueSheet> m_tempCueSheet;
};
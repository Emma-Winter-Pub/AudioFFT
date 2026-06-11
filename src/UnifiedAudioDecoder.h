#pragma once

#include "DecoderTypes.h"
#include "FftTypes.h"
#include "CueParser.h"

#include <QObject>
#include <QString>
#include <vector>
#include <optional>

struct AVFormatContext;
struct AVChannelLayout;

class UnifiedAudioDecoder : public QObject {
    Q_OBJECT
public:
    explicit UnifiedAudioDecoder(QObject* parent = nullptr);
    bool decode(const QString& filePath, 
                int targetTrackIdx = -1, 
                int targetChannelIdx = -1,
                double startSec = 0.0, 
                double endSec = 0.0,
                std::optional<CueSheet> cueSheet = std::nullopt);
    const PcmDataVariant& getPcmData() const { return m_pcmData; }    
    const DecoderTypes::AudioMetadata& getMetadata() const { return m_metadata; }

signals:
    void logMessage(const QString& msg);

private:
    bool decodeStandard(const QString& filePath, int trackIndex, int targetChannelIdx, double startSec, double endSec);   
    void logFileDetails(AVFormatContext* formatContext);
    static QString getChannelLayoutString(const AVChannelLayout* ch_layout);
    bool buildChannelMatrix(SwrContext* swr, int inputChannels, int targetChannelIdx);
    PcmDataVariant m_pcmData;    
    DecoderTypes::AudioMetadata m_metadata;
    std::optional<CueSheet> m_tempCueSheet;
};
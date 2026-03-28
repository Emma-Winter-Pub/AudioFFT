#pragma once

#include "FftTypes.h"
#include "FFmpegMemLoader.h"

#include <QString>
#include <vector>
#include <optional>

struct AVFormatContext;

struct BatDecodedAudio {
    PcmDataVariant pcmData; 
    int sampleRate = 0;
    double durationSec = 0.0;
    int sourceBitDepth = 0;
};

class BatAudioDecoder {
public:
    BatAudioDecoder();
    ~BatAudioDecoder();
    std::optional<BatDecodedAudio> decode(const QString& filePath);
    std::optional<BatDecodedAudio> decode(SharedFileBuffer buffer);

private:
    std::optional<BatDecodedAudio> decodeInternal(AVFormatContext* formatContext);
};
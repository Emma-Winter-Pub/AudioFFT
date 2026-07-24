#pragma once

#include "FFTTypes.h"
#include "FFmpegMemLoader.h"

#include <QString>
#include <vector>
#include <optional>

struct AVFormatContext;

struct BatchFullLoadDecodedAudio {
    PCMDataVariant pcmData; 
    int sampleRate = 0;
    double durationSec = 0.0;
    int sourceBitDepth = 0;
};

class BatchFullLoadAudioDecoder {
public:
    BatchFullLoadAudioDecoder();
    ~BatchFullLoadAudioDecoder();
    std::optional<BatchFullLoadDecodedAudio> decode(const QString& filePath);
    std::optional<BatchFullLoadDecodedAudio> decode(SharedFileBuffer buffer);

private:
    std::optional<BatchFullLoadDecodedAudio> decodeInternal(AVFormatContext* formatContext);
};
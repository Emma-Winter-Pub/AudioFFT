#pragma once

#include <QString>
#include <vector>
#include <optional>


struct BatDecodedAudio {
    std::vector<float> pcmData;
    int sampleRate     = 0;
    double durationSec = 0.0;
};


class BatAudioDecoder
{
public:
    BatAudioDecoder();
    ~BatAudioDecoder();
    std::optional<BatDecodedAudio> decode(const QString& filePath);
};
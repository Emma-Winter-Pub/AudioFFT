#pragma once

#include "AudioDecoderTypes.h"
#include "FFTTypes.h"

#include <vector>

enum class ExecutionMode {
    LoadToRam,  
    DirectRead  
};

class XAudioDecoder {
public:
    virtual ~XAudioDecoder() = default;
    virtual bool execute(
        const QString& filePath, 
        int trackIndex, 
        int channelIndex,
        int sourceBitDepth,
        PCMDataVariant& outPcmData,
        AudioDecoderTypes::AudioMetadata& outMetadata, 
        AudioDecoderTypes::LogCallback logCb, 
        ExecutionMode mode,
        double startSec = 0.0,
        double endSec = 0.0
    ) = 0;
};
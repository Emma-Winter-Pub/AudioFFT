#pragma once

#include "DecoderTypes.h"
#include "FftTypes.h"

#include <vector>

enum class ExecutionMode {
    LoadToRam,  
    DirectRead  
};

class IParallelPipeline {
public:
    virtual ~IParallelPipeline() = default;
    virtual bool execute(
        const QString& filePath, 
        int trackIndex, 
        int channelIndex,
        int sourceBitDepth,
        PcmDataVariant& outPcmData,
        DecoderTypes::AudioMetadata& outMetadata, 
        DecoderTypes::LogCallback logCb, 
        ExecutionMode mode,
        double startSec = 0.0,
        double endSec = 0.0
    ) = 0;
};
#pragma once

#include "IParallelPipeline.h"

#include <QCoreApplication>

class FlacPipeline : public IParallelPipeline {
    Q_DECLARE_TR_FUNCTIONS(FlacPipeline)

public:
    bool execute(
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
    ) override;
};
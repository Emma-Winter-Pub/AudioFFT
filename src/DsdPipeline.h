#pragma once

#include "IParallelPipeline.h"

#include <QCoreApplication>

class DsdPipeline : public IParallelPipeline {
    Q_DECLARE_TR_FUNCTIONS(DsdPipeline)

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
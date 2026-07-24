#pragma once

#include "XAudioDecoder.h"

#include <QCoreApplication>

class AudioDecoderFLAC : public XAudioDecoder {
    Q_DECLARE_TR_FUNCTIONS(AudioDecoderFLAC)

public:
    bool execute(
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
    ) override;
};
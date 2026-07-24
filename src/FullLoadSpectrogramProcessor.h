#pragma once

#include "MappingCurves.h"
#include "ColorPaletteFactory.h"
#include "FFTWindowFunctions.h"
#include "AudioDecoderTypes.h"
#include "FFTTypes.h"
#include "CueParser.h"

#include <QObject>
#include <QString>
#include <QImage>
#include <vector>
#include <optional>

enum class ProcessMode {
    Full,
    DecodeOnly
};
Q_DECLARE_METATYPE(ProcessMode)

class FullLoadSpectrogramProcessor : public QObject {
    Q_OBJECT

public:
    explicit FullLoadSpectrogramProcessor(QObject *parent = nullptr);

public slots:
    void processFile(const QString &filePath,
                     double timeInterval, int imageHeight, int fftSize,
                     CurveType curveType, double minDb, double maxDb,
                     const QString& paletteId, bool paletteInverted, bool paletteNegative,
                     FFTWindowType windowType,
                     int targetTrackIdx, int targetChannelIdx,
                     double startSec = 0.0, double endSec = 0.0,
                     ProcessMode mode = ProcessMode::Full,
                     std::optional<CueSheet> cueSheet = std::nullopt);
    void reProcessFromPcm(
        const PCMDataVariant& pcmData,
        const AudioDecoderTypes::AudioMetadata& metadata,
        double timeInterval,
        int imageHeight,
        int fftSize,
        CurveType curveType,
        double minDb,
        double maxDb,
        const QString& paletteId,
        bool paletteInverted,
        bool paletteNegative,
        FFTWindowType windowType
    );
    void reProcessFromFft(
        const SpectrumDataVariant& spectrumData,
        const AudioDecoderTypes::AudioMetadata& metadata,
        int fftSize,
        int imageHeight,
        int sampleRate,
        CurveType curveType,
        double minDb,
        double maxDb,
        const QString& paletteId,
        bool paletteInverted,
        bool paletteNegative
    );

signals:
    void logMessage(const QString &message);
    void processingFinished(
        const QImage &spectrogram,
        const PCMDataVariant& pcmData,
        const SpectrumDataVariant& spectrumData, 
        const AudioDecoderTypes::AudioMetadata& metadata
    );    
    void processingFailed(const QString &errorMessage);
};
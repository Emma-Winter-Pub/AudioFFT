#pragma once

#include "MappingCurves.h"
#include "ColorPalette.h"
#include "WindowFunctions.h"
#include "DecoderTypes.h"
#include "FftTypes.h"
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

class SpectrogramProcessor : public QObject {
    Q_OBJECT

public:
    explicit SpectrogramProcessor(QObject *parent = nullptr);

public slots:
    void processFile(const QString &filePath,
                     double timeInterval, int imageHeight, int fftSize,
                     CurveType curveType, double minDb, double maxDb,
                     PaletteType paletteType, WindowType windowType,
                     int targetTrackIdx, int targetChannelIdx,
                     double startSec = 0.0, double endSec = 0.0,
                     ProcessMode mode = ProcessMode::Full,
                     std::optional<CueSheet> cueSheet = std::nullopt);
    void reProcessFromPcm(
        const PcmDataVariant& pcmData,
        const DecoderTypes::AudioMetadata& metadata,
        double timeInterval,
        int imageHeight,
        int fftSize,
        CurveType curveType,
        double minDb,
        double maxDb,
        PaletteType paletteType,
        WindowType windowType
    );
    void reProcessFromFft(
        const SpectrumDataVariant& spectrumData,
        const DecoderTypes::AudioMetadata& metadata,
        int fftSize,
        int imageHeight,
        int sampleRate,
        CurveType curveType,
        double minDb,
        double maxDb,
        PaletteType paletteType
    );

signals:
    void logMessage(const QString &message);
    void processingFinished(
        const QImage &spectrogram,
        const PcmDataVariant& pcmData,
        const SpectrumDataVariant& spectrumData, 
        const DecoderTypes::AudioMetadata& metadata
    );    
    void processingFailed(const QString &errorMessage);
};
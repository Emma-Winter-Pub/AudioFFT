#pragma once

#include "SpectrogramWidgetBase.h"
#include "ImageExporter.h"
#include "AudioDecoderTypes.h" 
#include "FFTTypes.h"
#include "XPlayerProvider.h"
#include "CueParser.h"
#include "FullLoadSpectrogramProcessor.h"
#include "CrosshairOverlay.h"
#include "FFTSpectrumDataProvider.h"

#include <QThread>
#include <QImage>
#include <vector>
#include <QFutureWatcher>
#include <variant>
#include <QSharedPointer>
#include <optional>

class FullLoadSpectrogramViewer;
class FullLoadSpectrogramProcessor;

struct FftStateCache {
    int fftSize = -1;
    FFTWindowType windowType = FFTWindowType::Hann;
    double interval = -1.0;
    int sampleRate = -1;
    bool isValid() const { return fftSize > 0 && sampleRate > 0; }
    void reset() { *this = FftStateCache(); }
};

class FullLoadWidget : public SpectrogramWidgetBase {
    Q_OBJECT

public:
    explicit FullLoadWidget(QWidget *parent = nullptr);
    ~FullLoadWidget();
    QString getCurrentFilePath() const { return m_currentlyProcessedFile; }
    void updateCrosshairStyle(const CrosshairStyle& style, bool enabled) override;
    void updateSpectrumProfileStyle(bool visible, const QColor& color, int lineWidth, bool filled, int alpha, SpectrumProfileType type, SpectrumProfileDirection direction) override;
    void updatePlayheadStyle(const PlayheadStyle& style) override;
    void setProfileFrameRate(int fps) override;
    void updateProbeConfig(DataSourceType spectrumSrc, DataSourceType probeSrc, int precision) override;
    void setIndicatorVisibility(bool showFreq, bool showTime, bool showDb) override;
    void applyGlobalPreferences(const GlobalPreferences& prefs, bool silent = false);
    void setAutoExpandOnPlay(bool enabled);
    void tryAutoExpand(bool isExternalOpen);
    void abortAutoExpand();

public slots:
    void setPlayheadPosition(double seconds);
    void setPlayheadVisible(bool visible);
    void loadFile(const QString& filePath);
    void handleProcessingFinished(
        const QImage &spectrogram,
        const PCMDataVariant& pcmData,
        const SpectrumDataVariant& spectrumData,
        const AudioDecoderTypes::AudioMetadata& metadata
    );    
    void handleProcessingFailed(const QString &errorMessage);
    void handleExportFinished(const ImageExporter::ExportResult &result);

signals:
    void startProcessing(const QString &filePath,
                         double timeInterval, int imageHeight, int fftSize,
                         CurveType curveType, double minDb, double maxDb,
                         const QString& paletteId, bool paletteInverted, bool paletteNegative,
                         FFTWindowType windowType,
                         int targetTrackIdx, int targetChannelIdx,
                         double startSec, double endSec,
                         ProcessMode mode,
                         std::optional<CueSheet> cueSheet = std::nullopt);
    void startReProcessing(
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
    void startReProcessingFromFft(
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
    void logMessageGenerated(const QString& message); 
    void filePathChanged(const QString& filePath);
    void playerProviderReady(QSharedPointer<XPlayerProvider> provider, double totalDurationSec);
    void seekRequested(double seconds);

protected:
    void retranslateBaseUi() override;

private slots:
    void onOpenFileClicked();
    void onSaveClicked();
    void triggerReprocessing();
    void onTrackActionTriggered(int trackIdx);
    void onChannelActionTriggered(int channelIdx);
    void onFileSelected(const QString &filePath);

private:
    void setupWorkerThread();
    void setupConnections();
    void updateAutoPrecision();
    bool hasPcmData() const;
    QString findAudioFileForCue(const QString& cuePath, const QString& cueAudioTarget);
    void updateTrackMenuFromCue();
    PCMDataVariant slicePcmData(const PCMDataVariant& source, int sampleRate, double startSec, double endSec);
    FullLoadSpectrogramViewer *m_spectrogramViewer;
    QThread m_workerThread;
    FullLoadSpectrogramProcessor *m_processor;
    QFutureWatcher<ImageExporter::ExportResult> m_exportWatcher;
    QImage m_currentSpectrogram;
    PCMDataVariant m_currentPcmData;
    SpectrumDataVariant m_cachedFftData;
    FFTSpectrumDataProvider* m_fftProvider = nullptr;
    QString m_currentlyProcessedFile;
    QString m_lastOpenPath;
    QString m_lastSavePath;
    AudioDecoderTypes::AudioMetadata m_currentMetadata;
    double m_currentAudioDuration = 0.0;
    QString m_preciseDurationStr;
    int m_currentSampleRate = 0;
    int m_currentTrackIdx = -1;
    int m_currentChannelIdx = -1;
    int m_currentFftSize = 0;
    bool m_isCueMode = false;
    std::optional<CueSheet> m_currentCueSheet;
    QString m_originalCueFilePath; 
    int m_currentCueTrackIndex = 0;
    PCMDataVariant m_fullCachedPcmData;
    bool m_isFullCacheAvailable = false;
    bool m_isLoadingFullForCue = false;
    FftStateCache m_fftStateCache;
};
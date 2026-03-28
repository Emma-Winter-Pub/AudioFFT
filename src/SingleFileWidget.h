#pragma once

#include "SpectrogramWidgetBase.h"
#include "ImageExporter.h"
#include "DecoderTypes.h" 
#include "FftTypes.h"
#include "IPlayerProvider.h"
#include "CueParser.h"
#include "SpectrogramProcessor.h"
#include "CrosshairOverlay.h"
#include "FftSpectrumDataProvider.h"

#include <QThread>
#include <QImage>
#include <vector>
#include <QFutureWatcher>
#include <variant>
#include <QSharedPointer>
#include <optional>

class SpectrogramViewer;
class SpectrogramProcessor;

struct FftStateCache {
    int fftSize = -1;
    WindowType windowType = WindowType::Hann;
    double interval = -1.0;
    int sampleRate = -1;
    bool isValid() const { return fftSize > 0 && sampleRate > 0; }
    void reset() { *this = FftStateCache(); }
};

class SingleFileWidget : public SpectrogramWidgetBase {
    Q_OBJECT

public:
    explicit SingleFileWidget(QWidget *parent = nullptr);
    ~SingleFileWidget();
    QString getCurrentFilePath() const { return m_currentlyProcessedFile; }
    void updateCrosshairStyle(const CrosshairStyle& style, bool enabled) override;
    void updateSpectrumProfileStyle(bool visible, const QColor& color, int lineWidth, bool filled, int alpha, SpectrumProfileType type) override;
    void updatePlayheadStyle(const PlayheadStyle& style) override;
    void setProfileFrameRate(int fps) override;
    void updateProbeConfig(DataSourceType spectrumSrc, DataSourceType probeSrc, int precision) override;
    void setIndicatorVisibility(bool showFreq, bool showTime, bool showDb) override;
    void applyGlobalPreferences(const GlobalPreferences& prefs, bool silent = false);

public slots:
    void setPlayheadPosition(double seconds);
    void setPlayheadVisible(bool visible);
    void handleProcessingFinished(
        const QImage &spectrogram,
        const PcmDataVariant& pcmData,
        const SpectrumDataVariant& spectrumData,
        const DecoderTypes::AudioMetadata& metadata
    );    
    void handleProcessingFailed(const QString &errorMessage);
    void handleExportFinished(const ImageExporter::ExportResult &result);

signals:
    void startProcessing(const QString &filePath,
                         double timeInterval, int imageHeight, int fftSize,
                         CurveType curveType, double minDb, double maxDb,
                         PaletteType paletteType, WindowType windowType,
                         int targetTrackIdx, int targetChannelIdx,
                         double startSec, double endSec,
                         ProcessMode mode,
                         std::optional<CueSheet> cueSheet = std::nullopt);
    void startReProcessing(
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
    void startReProcessingFromFft(
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
    void logMessageGenerated(const QString& message); 
    void filePathChanged(const QString& filePath);
    void playerProviderReady(QSharedPointer<IPlayerProvider> provider, double totalDurationSec);
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
    PcmDataVariant slicePcmData(const PcmDataVariant& source, int sampleRate, double startSec, double endSec);
    SpectrogramViewer *m_spectrogramViewer;
    QThread m_workerThread;
    SpectrogramProcessor *m_processor;
    QFutureWatcher<ImageExporter::ExportResult> m_exportWatcher;
    QImage m_currentSpectrogram;
    PcmDataVariant m_currentPcmData;
    SpectrumDataVariant m_cachedFftData;
    FftSpectrumDataProvider* m_fftProvider = nullptr;
    QString m_currentlyProcessedFile;
    QString m_lastOpenPath;
    QString m_lastSavePath;
    DecoderTypes::AudioMetadata m_currentMetadata;
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
    PcmDataVariant m_fullCachedPcmData;
    bool m_isFullCacheAvailable = false;
    bool m_isLoadingFullForCue = false;
    FftStateCache m_fftStateCache;
};
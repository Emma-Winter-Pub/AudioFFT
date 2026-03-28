#pragma once

#include "SpectrogramWidgetBase.h"
#include "FastConfig.h"
#include "FastTypes.h"
#include "FastUtils.h"
#include "IPlayerProvider.h"
#include "CueParser.h"
#include "CrosshairOverlay.h"

#include <QWidget>
#include <QFutureWatcher>
#include <QImage>
#include <QSharedPointer>
#include <optional>

class FastSpectrogramViewer;
class FastSpectrogramProcessor;

class FastStreamingWidget : public SpectrogramWidgetBase {
    Q_OBJECT

public:
    explicit FastStreamingWidget(QWidget *parent = nullptr);
    ~FastStreamingWidget();
    QString getCurrentFilePath() const { return m_currentFilePath; }
    void updateCrosshairStyle(const CrosshairStyle& style, bool enabled) override;
    void updateSpectrumProfileStyle(bool visible, const QColor& color, int lineWidth, bool filled, int alpha, SpectrumProfileType type) override;
    void updatePlayheadStyle(const PlayheadStyle& style) override;
    void setProfileFrameRate(int fps) override;
    void updateProbeConfig(DataSourceType spectrumSrc, DataSourceType probeSrc, int precision) override;
    void setIndicatorVisibility(bool showFreq, bool showTime, bool showDb) override;
    void applyGlobalPreferences(const GlobalPreferences& prefs, bool silent = false);

signals:
    void logMessageGenerated(const QString& message);
    void filePathChanged(const QString& filePath);
    void playerProviderReady(QSharedPointer<IPlayerProvider> provider, double totalDurationSec);
    void seekRequested(double seconds);

public slots:
    void setPlayheadPosition(double seconds);
    void setPlayheadVisible(bool visible);

protected:
    void retranslateBaseUi() override;

private slots:
    void onOpenFileClicked();
    void onSaveClicked();
    void restartStream();
    void onTrackActionTriggered(int trackIdx);
    void onChannelActionTriggered(int channelIdx);
    void onProcessingStarted(const FastTypes::FastAudioMetadata& metadata);
    void onChunkReady(const QImage& chunk, double startTime, double duration);
    void onProcessingFinished(double realDuration);
    void onProcessingFailed(const QString& msg);

private:
    void setupConnections();
    void startStream();
    void stopStream();
    void updateAutoPrecision();
    QImage generateFinalImageForSave();
    void handleFileOpen(const QString& filePath);
    QString findAudioFileForCue(const QString& cuePath, const QString& cueAudioTarget);
    void updateTrackMenuFromCue();
    FastSpectrogramViewer* m_viewer;
    FastSpectrogramProcessor* m_processor;
    QString m_currentFilePath;
    QString m_lastOpenPath;
    QString m_lastSavePath;
    FastTypes::FastAudioMetadata m_currentMetadata;
    QString m_preciseDurationStr;
    int m_currentTrackIdx = -1;
    int m_currentChannelIdx = -1;
    bool m_isCueMode = false;
    std::optional<CueSheet> m_currentCueSheet;
    QString m_originalCueFilePath;
    int m_currentCueTrackIndex = 0;
};
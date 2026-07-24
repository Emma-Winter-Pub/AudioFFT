#pragma once

#include "SpectrogramWidgetBase.h"
#include "StreamingConfig.h"
#include "StreamingTypes.h"
#include "StreamingUtils.h"
#include "XPlayerProvider.h"
#include "CueParser.h"
#include "CrosshairOverlay.h"

#include <QWidget>
#include <QFutureWatcher>
#include <QImage>
#include <QSharedPointer>
#include <optional>

class StreamingSpectrogramViewer;
class StreamingSpectrogramProcessor;

class StreamingWidget : public SpectrogramWidgetBase {
    Q_OBJECT

public:
    explicit StreamingWidget(QWidget *parent = nullptr);
    ~StreamingWidget();
    QString getCurrentFilePath() const { return m_currentFilePath; }
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

signals:
    void logMessageGenerated(const QString& message);
    void filePathChanged(const QString& filePath);
    void playerProviderReady(QSharedPointer<XPlayerProvider> provider, double totalDurationSec);
    void seekRequested(double seconds);

public slots:
    void setPlayheadPosition(double seconds);
    void setPlayheadVisible(bool visible);
    void loadFile(const QString& filePath);

protected:
    void retranslateBaseUi() override;

private slots:
    void onOpenFileClicked();
    void onSaveClicked();
    void restartStream();
    void onTrackActionTriggered(int trackIdx);
    void onChannelActionTriggered(int channelIdx);
    void onProcessingStarted(const StreamingTypes::StreamingAudioMetadata& metadata);
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
    StreamingSpectrogramViewer* m_viewer;
    StreamingSpectrogramProcessor* m_processor;
    QString m_currentFilePath;
    QString m_lastOpenPath;
    QString m_lastSavePath;
    StreamingTypes::StreamingAudioMetadata m_currentMetadata;
    QString m_preciseDurationStr;
    int m_currentTrackIdx = -1;
    int m_currentChannelIdx = -1;
    bool m_isCueMode = false;
    std::optional<CueSheet> m_currentCueSheet;
    QString m_originalCueFilePath;
    int m_currentCueTrackIndex = 0;
};
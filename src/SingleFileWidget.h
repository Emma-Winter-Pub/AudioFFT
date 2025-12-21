#pragma once

#include "ImageExporter.h"
#include <QFutureWatcher>
#include "MappingCurves.h"

#include <QWidget>
#include <QThread>
#include <QImage>
#include <vector>


class QPushButton;
class QCheckBox;
class QSpinBox;
class QComboBox;
class SpectrogramViewer;
class SpectrogramProcessor;


class SingleFileWidget : public QWidget {
    Q_OBJECT
public:
    explicit SingleFileWidget(QWidget *parent = nullptr);
    ~SingleFileWidget();

    QCheckBox* getShowLogCheckBox() const { return m_showLogCheckBox; }
    QString getCurrentFilePath() const { return m_currentlyProcessedFile; }

public slots:
    void setShowGrid(bool show);
    void setVerticalZoomEnabled(bool enabled);
    void setCurveType(CurveType type);

signals:
    void startProcessing(const QString &filePath, double timeInterval, int imageHeight, int fftSize, CurveType curveType);
    void logMessageGenerated(const QString& message); 

    void startReProcessing(
        const std::vector<float>& pcmData,
        double durationSec,
        const QString& preciseDurationStr,
        int nativeSampleRate,
        double timeInterval,
        int imageHeight,
        int fftSize,
        CurveType curveType
    );
    void filePathChanged(const QString& filePath);

private slots:
    void saveSpectrogram();
    void onFileSelected(const QString &filePath);
    void onOpenFileClicked();

    void handleProcessingFinished(
        const QImage &spectrogram,
        const std::vector<float>& pcmData,
        double durationSec,
        const QString& preciseDurationStr,
        int nativeSampleRate
    );

    void handleProcessingFailed(const QString &errorMessage);
    void onProcessingParamsChanged();
    void onHeightParamsChanged();
    void onTimeIntervalChanged();
    void handleExportFinished(const ImageExporter::ExportResult &result);

private:
    void setupUI();
    void setupWorkerThread();
    void setControlsEnabled(bool enabled);
    int getRequiredFftSize(int height) const;
    
    QPushButton *m_openButton;
    QPushButton *m_saveButton;
    QCheckBox *m_showLogCheckBox;
    QCheckBox *m_showGridCheckBox;
    QCheckBox *m_enableVerticalZoomCheckBox;
    QCheckBox *m_enableWidthLimitCheckBox; 
    SpectrogramViewer *m_spectrogramViewer;
    QSpinBox* m_maxWidthSpinBox;
    QComboBox* m_imageHeightComboBox;
    QComboBox* m_timeIntervalComboBox;
    
    QThread m_workerThread;
    SpectrogramProcessor *m_processor;

    QFutureWatcher<ImageExporter::ExportResult> m_exportWatcher;

    QImage m_currentSpectrogram;
    double m_currentAudioDuration;
    QString m_preciseDurationStr;
    int m_currentSampleRate;
    int m_currentSpectrogramHeight;
    int m_currentFftSize;
    double m_currentTimeInterval;
    QString m_currentlyProcessedFile;
    std::vector<float> m_currentPcmData;
    
    QString m_lastSavePath; 
    QString m_lastOpenPath;

    CurveType m_currentCurveType;
};
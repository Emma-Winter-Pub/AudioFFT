#pragma once

#include "BatTypes.h"

#include <QWidget>

class QCheckBox;
class QLineEdit;
class QPushButton;
class QSpinBox;
class QComboBox;
class QTextEdit;
class QFormLayout;
class QLabel;
class QSlider;


class BatWidget : public QWidget {
    Q_OBJECT

public:
    explicit BatWidget(QWidget *parent = nullptr);
    ~BatWidget();

signals:
    void startBatchRequested(const BatSettings& settings);
    void pauseBatchRequested();
    void resumeBatchRequested();
    void stopBatchRequested();

public slots:
    void appendLog(const QString& message);
    void updateProgress(int current, int total);
    void onBatchFinished(const QString& summaryReport);
    void onBatchStarted();
    void onBatchPaused();
    void onBatchResumed();
    void onBatchStopped();

private slots:
    void onBrowseInputPath();
    void onBrowseOutputPath();
    void onStartPauseResumeClicked();
    void onStopClicked();
    void onFormatChanged(const QString& format);

private:
    void setupUi();
    void setControlsEnabledForBatch(bool isRunning);
    
    enum class BatchState { Idle, Running, Paused };
    BatchState m_currentState = BatchState::Idle;

    QPushButton* m_inputPathSelectButton;
    QLabel*      m_inputPathDisplayLabel;
    QPushButton* m_outputPathSelectButton;
    QLabel*      m_outputPathDisplayLabel;

    QCheckBox* m_includeSubfoldersCheckBox;
    QCheckBox* m_reuseSubfoldersCheckBox;
    QComboBox* m_formatComboBox;
    QLabel* m_qualityLabel;
    QSlider* m_qualitySlider;
    QLabel* m_qualityValueLabel;
    QCheckBox* m_gridCheckBox;
    QCheckBox* m_widthLimitCheckBox;
    QSpinBox* m_maxWidthSpinBox;
    QComboBox* m_heightComboBox;
    QComboBox* m_timeIntervalComboBox;
    QPushButton* m_startPauseResumeButton;
    QPushButton* m_stopButton;
    QTextEdit* m_logTextEdit;

    QString m_lastInputPath;
    QString m_lastOutputPath;
};
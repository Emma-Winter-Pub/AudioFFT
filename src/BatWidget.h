#pragma once

#include "BatTypes.h"
#include "BatchStreamTypes.h"

#include <QWidget>
#include <QSharedPointer>
#include <QEvent>

class QLineEdit;
class QPushButton;
class QTextEdit;

class BatWidget : public QWidget {
    Q_OBJECT

public:
    explicit BatWidget(QWidget *parent = nullptr);
    ~BatWidget();
    void setPaletteType(PaletteType type);

signals:
    void requestScan(const BatSettings& settings);
    void requestStartProcessing();
    void pauseBatchRequested();
    void resumeBatchRequested();
    void stopBatchRequested();
    void requestScanStream(const BatchStreamSettings& settings);
    void requestStartProcessingStream();
    void pauseBatchStreamRequested();
    void resumeBatchStreamRequested();
    void stopBatchStreamRequested();

public slots:
    void appendLog(const QString& message);
    void updateProgress(int current, int total);
    void onBatchFinished(const QString& summaryReport);
    void onBatchStarted();
    void onBatchPaused();
    void onBatchResumed();
    void onBatchStopped();
    void onScanFinished(QSharedPointer<FileSnapshot> snapshot);

protected:
    void changeEvent(QEvent *event) override;

private slots:
    void onBrowseInputPath();
    void onBrowseOutputPath();
    void onSettingsClicked();
    void onStartClicked();
    void onStopClicked();

private:
    void setupUi();
    void retranslateUi();
    void setControlsEnabledForBatch(bool isRunning);
    void updateButtonSizes();
    BatchStreamSettings convertToStreamSettings(const BatSettings& s) const;
    enum class BatchState { Idle, Running, Paused };
    BatchState m_currentState = BatchState::Idle;
    BatSettings m_currentSettings;
    bool m_hasFinishedOnce = false;         
    BatSettings m_lastFinishedSettings;
    QSharedPointer<FileSnapshot> m_lastFinishedSnapshot;    
    QSharedPointer<FileSnapshot> m_currentRunningSnapshot;
    QLineEdit*   m_editInput;
    QLineEdit*   m_editOutput;
    QPushButton* m_btnInput;
    QPushButton* m_btnOutput;
    QPushButton* m_btnSettings;
    QPushButton* m_btnStart;
    QPushButton* m_btnStop;
    QTextEdit*   m_logEdit;
};
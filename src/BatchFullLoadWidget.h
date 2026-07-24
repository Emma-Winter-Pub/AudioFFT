#pragma once

#include "BatchFullLoadTypes.h"
#include "BatchStreamTypes.h"

#include <QWidget>
#include <QSharedPointer>
#include <QEvent>

class QLineEdit;
class QPushButton;
class QTextEdit;
class LogListView;
class LogListModel;

class BatchFullLoadWidget : public QWidget {
    Q_OBJECT

public:
    explicit BatchFullLoadWidget(QWidget *parent = nullptr);
    ~BatchFullLoadWidget();
    void setPalette(const QString& id, bool inverted, bool negative);
    bool isBusy() const { return m_currentState != BatchState::Idle; }

signals:
    void requestScan(const BatchSettings& settings);
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
    BatchStreamSettings convertToStreamSettings(const BatchSettings& s) const;
    enum class BatchState { Idle, Running, Paused };
    BatchState m_currentState = BatchState::Idle;
    BatchSettings m_currentSettings;
    bool m_hasFinishedOnce = false;         
    BatchSettings m_lastFinishedSettings;
    QSharedPointer<FileSnapshot> m_lastFinishedSnapshot;    
    QSharedPointer<FileSnapshot> m_currentRunningSnapshot;
    QLineEdit* m_editInput;
    QLineEdit* m_editOutput;
    QPushButton* m_btnInput;
    QPushButton* m_btnOutput;
    QPushButton* m_btnSettings;
    QPushButton* m_btnStart;
    QPushButton* m_btnStop;
    LogListView* m_logView;
    LogListModel* m_logModel;
};
#pragma once

#include "BatchFullLoadTypes.h"
#include "BatchFullLoadProcessor.h" 

#include <QObject>
#include <QElapsedTimer>
#include <atomic>

class BatchFullLoadWorker : public QObject {
    Q_OBJECT

public:
    explicit BatchFullLoadWorker(int bucketId, int bucketIdPadding, std::atomic<bool>* stopFlag, std::atomic<bool>* pauseFlag, BatchFullLoadProcessor* processor, QObject* parent = nullptr);
    void setProcessor(BatchFullLoadProcessor* processor) { m_processor = processor; }
    void setDirectWriteMode(bool direct) { m_directWriteMode = direct; }

public slots:
    void startWorkLoop(const BatchSettings& settings);

signals:
    void logMessage(const QString& msg);
    void fileCompleted(const QString& filePath, bool success, const QString& errorMsg, const FilePerformanceMetrics& metrics, int threadId);
    void bucketFinished(int bucketId, qint64 elapsedMs);
    void fileMetricsReported(const FilePerformanceMetrics& metrics);

private:
    void checkPauseState();
    bool processSingleFile(BatchTask& task, const BatchSettings& settings, QString& errorMsg, bool& outIsResized, FilePerformanceMetrics& outMetrics);
    int getRequiredFftSize(int height) const;
    int m_bucketId;
    int m_bucketIdPadding;
    std::atomic<bool>* m_stopFlag;
    std::atomic<bool>* m_pauseFlag;
    BatchFullLoadProcessor* m_processor = nullptr;
    QElapsedTimer m_bucketTimer;
    bool m_directWriteMode = true; 
};
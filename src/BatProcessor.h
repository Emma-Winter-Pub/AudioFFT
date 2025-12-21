#pragma once

#include "BatTypes.h"

#include <mutex>
#include <atomic>
#include <optional>
#include <QMap>
#include <QList>
#include <QObject>
#include <QThread>
#include <QElapsedTimer>


class BatWorker;


class BatProcessor : public QObject {
    Q_OBJECT

public:
    explicit BatProcessor(QObject *parent = nullptr);
    ~BatProcessor();

public slots:
    void processBatch(const BatSettings& settings);
    void pause();
    void resume();
    void stop();

signals:
    void logMessage(const QString& message);
    void progressUpdated(int processedCount, int totalCount);
    void batchFinished(const QString& summaryReport);
    void batchStarted();
    void batchPaused();
    void batchResumed();
    void batchStopped();

private slots:
    void onWorkerFileCompleted(const QString& filePath, bool success, const QString& errorInfo);
    void onWorkerLog(const QString& message);
    void onWorkerFinished();
    void onBucketFinished(int bucketId, qint64 elapsedMs);
    void onFileMetricsReported(const FilePerformanceMetrics& metrics);

private:
    QList<FileInfo> scanAndProbeFiles(const QString& rootPath, bool includeSubfolders);
    std::optional<FileInfo> probeFile(const QString& path);
    void startWorkers(QList<FileInfo>& allFiles, const BatSettings& settings); 
    void cleanup();

    QList<QThread*> m_workerThreads;
    QList<BatWorker*> m_workers;
    
    BatSettings m_currentSettings;
    std::atomic<bool> m_stopFlag;
    std::atomic<bool> m_pauseFlag;
    std::atomic<int> m_totalFileCount;
    std::atomic<int> m_processedFileCount;
    std::atomic<int> m_runningWorkers;
    std::mutex m_failedFilesMutex;
    QStringList m_failedFiles;
    QElapsedTimer m_totalTaskTimer;
    
    QList<FilePerformanceMetrics> m_allFileMetrics;
    std::mutex m_metricsMutex;

    QMap<int, qint64> m_bucketCompletionTimes;
    std::mutex m_bucketDataMutex;
    int m_bucketIdPadding;
};
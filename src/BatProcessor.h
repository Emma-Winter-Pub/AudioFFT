#pragma once

#include "BatTypes.h"
#include "BatIoScheduler.h" 
#include "BatchStreamBuffer.h"

#include <QObject>
#include <QThread>
#include <QList>
#include <QStringList>
#include <QMap>
#include <QHash>
#include <QQueue>
#include <QElapsedTimer>
#include <QSharedPointer>
#include <mutex>
#include <atomic>
#include <optional>
#include <condition_variable>
#include <memory> 

class BatWorker;

class BatProcessor : public QObject
{
    Q_OBJECT

public:
    explicit BatProcessor(QObject *parent = nullptr);
    ~BatProcessor();
    std::optional<BatTask> claimNextTask();
    void notifyLargeFileFinished();
    void enqueueWriteTask(BatWriteTask task);

public slots:
    void scanDirectory(const BatSettings& settings);
    void startProcessing();
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
    void scanFinished(QSharedPointer<FileSnapshot> snapshot);

private slots:
    void onWorkerFileCompleted(const QString& filePath, bool success, const QString& errorInfo, const FilePerformanceMetrics& metrics, int threadId);
    void onWorkerLog(const QString& message);
    void onWorkerFinished();
    void onBucketFinished(int bucketId, qint64 elapsedMs);
    void onFileMetricsReported(const FilePerformanceMetrics& metrics);
    void onIoThreadFinished(); 
    void onWriterThreadFinished(); 

private:
    QList<FileInfo> scanAndProbeFiles(const QString& rootPath, bool includeSubfolders);
    std::optional<FileInfo> probeFile(const QString& path);
    void startWorkers(QList<FileInfo>& allFiles, const BatSettings& settings, BatExecutionPlan plan); 
    void cleanup();
    void runReaderLoop(QList<FileInfo> files); 
    void runWriterLoop();                      
    void runHybridLoop(QList<FileInfo> files); 
    QList<QThread*> m_workerThreads;
    QList<BatWorker*> m_workers;
    QThread* m_ioThread = nullptr;     
    QThread* m_writerThread = nullptr; 
    BatSettings m_currentSettings;
    QList<FileInfo> m_pendingFiles;
    std::atomic<bool> m_stopFlag;
    std::atomic<bool> m_pauseFlag;
    std::atomic<int> m_totalFileCount;
    std::atomic<int> m_processedFileCount;
    std::atomic<int> m_runningWorkers;
    std::mutex m_queueMutex;
    QQueue<FileInfo> m_taskQueue;
    std::mutex m_ioMutex;
    std::condition_variable m_ioCV;
    QQueue<BatTask> m_bufferPool;
    std::atomic<int64_t> m_currentPoolBytes{0};
    bool m_ioFinished = false;
    bool m_largeFileInProgress = false;
    bool m_isHddMode = false; 
    std::mutex m_writeMutex;
    std::condition_variable m_writeCV;
    QQueue<BatWriteTask> m_writeQueue;
    std::atomic<int64_t> m_currentWritePoolBytes{0}; 
    std::mutex m_failedFilesMutex;
    QStringList m_failedFiles;
    std::mutex m_resizedFilesMutex;
    QStringList m_resizedFiles;
    QElapsedTimer m_totalTaskTimer;
    QList<FilePerformanceMetrics> m_allFileMetrics;
    std::mutex m_metricsMutex;
    QMap<int, qint64> m_bucketCompletionTimes;
    std::mutex m_bucketDataMutex;
    int m_bucketIdPadding;
};
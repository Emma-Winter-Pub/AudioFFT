#pragma once

#include "BatchFullLoadTypes.h"
#include "BatchFullLoadIoScheduler.h" 
#include "BatchStreamBuffer.h"

#include <QObject>
#include <QThread>
#include <QList>
#include <QStringList>
#include <QMap>
#include <QHash>
#include <QQueue>
#include <QFuture>
#include <QElapsedTimer>
#include <QSharedPointer>
#include <mutex>
#include <atomic>
#include <optional>
#include <condition_variable>
#include <memory> 

class BatchFullLoadWorker;

class BatchFullLoadProcessor : public QObject
{
    Q_OBJECT

public:
    explicit BatchFullLoadProcessor(QObject *parent = nullptr);
    ~BatchFullLoadProcessor();
    std::optional<BatchTask> claimNextTask();
    void notifyLargeFileFinished();
    void enqueueWriteTask(BatchWriteTask task);
    void releaseReadBuffer(size_t size);

public slots:
    void scanDirectory(const BatchSettings& settings);
    void startProcessing();
    void pause();
    void resume();
    void stop();
    void onAsyncWriteCompleted(const QString& filePath, bool success, const QString& errorInfo, const FilePerformanceMetrics& metrics, int threadId);

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
    std::optional<FileInfo> probeFile(const QString& path, bool& outIsVideo);
    void startWorkers(QList<FileInfo>& allFiles, const BatchSettings& settings, BatchExecutionPlan plan); 
    void cleanup();
    void runReaderLoop(QList<FileInfo> files);
    void runWriterLoop();
    void runHybridLoop(QList<FileInfo> files);
    void checkAndFinishBatch();
    QList<QThread*> m_workerThreads;
    QList<BatchFullLoadWorker*> m_workers;
    QThread* m_ioThread = nullptr;
    QThread* m_writerThread = nullptr;
    BatchSettings m_currentSettings;
    QList<FileInfo> m_pendingFiles;
    std::atomic<bool> m_stopFlag{false};
    std::atomic<bool> m_pauseFlag{false};
    std::atomic<bool> m_workersDone{false};
    std::atomic<bool> m_isCleaningUp{false};
    std::atomic<int> m_totalFileCount;
    std::atomic<int> m_processedFileCount;
    std::atomic<int> m_runningWorkers;
    std::mutex m_queueMutex;
    QQueue<FileInfo> m_taskQueue;
    std::mutex m_ioMutex;
    std::condition_variable m_ioCV;
    QQueue<BatchTask> m_bufferPool;
    std::atomic<int64_t> m_currentPoolBytes{0};
    bool m_ioFinished = false;
    bool m_largeFileInProgress = false;
    bool m_isHddMode = false; 
    std::mutex m_writeMutex;
    std::condition_variable m_writeCV;
    QQueue<BatchWriteTask> m_writeQueue;
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
    QFuture<BatchExecutionPlan> m_planFuture;
    std::mutex m_listMutex;
    QStringList m_invalidWhiteListFiles;
    QStringList m_unexpectedValidFiles;
    QStringList m_videoFilesWithAudioExt;
};
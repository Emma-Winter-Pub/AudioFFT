#pragma once

#include "BatchStreamTypes.h"
#include "BatchStreamBuffer.h"
#include "BatchStreamIoScheduler.h"

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

class BatchStreamWorker;

struct BatchStreamScannedFile {
    QString path;
    qint64 duration;
    int codecId;
    QString codecName;
    qint64 fileSize;
};

struct BatchStreamTask {
    BatchStreamScannedFile fileInfo;
    std::shared_ptr<BatchStreamBuffer> buffer;
};

using BatchStreamFileSnapshot = QHash<QString, qint64>;

class BatchStreamProcessor : public QObject {
    Q_OBJECT

public:
    explicit BatchStreamProcessor(QObject *parent = nullptr);
    ~BatchStreamProcessor();
    std::optional<BatchStreamTask> claimTask(int workerId);
    void enqueueWriteTask(BatchStreamWriteTask task);

public slots:
    void scanDirectory(const BatchStreamSettings& settings);
    void startProcessing();
    void pause();
    void resume();
    void stop();

signals:
    void logMessage(const QString& message);
    void progressUpdated(int processedCount, int totalCount);
    void scanFinished(QSharedPointer<BatchStreamFileSnapshot> snapshot);
    void batchStarted();
    void batchPaused();
    void batchResumed();
    void batchStopped();
    void batchFinished(const QString& summaryReport);

private slots:
    void onWorkerFileFinished(const QString& filePath, bool success, const QString& errorInfo, int threadId, qint64 elapsedMs);
    void onWorkerLog(const QString& message);
    void onWorkerBucketFinished(int bucketId, qint64 elapsedMs);
    void onWorkerThreadFinished();
    void onIoThreadFinished();
    void onWriterThreadFinished();

private:
    QList<BatchStreamScannedFile> performScan(const QString& rootPath, bool includeSubfolders);
    std::optional<BatchStreamScannedFile> probeFile(const QString& path);
    void startWorkers(QList<BatchStreamScannedFile>& allFiles);
    void cleanupThreads();
    void runReaderLoop(QList<BatchStreamScannedFile> files, int concurrency);
    void runWriterLoop();
    void runHybridLoop(QList<BatchStreamScannedFile> files, int concurrency);
    BatchStreamSettings m_currentSettings;
    QList<BatchStreamScannedFile> m_pendingFiles;
    QList<QThread*> m_threads;
    QList<BatchStreamWorker*> m_workers;
    std::atomic<bool> m_stopFlag;
    std::atomic<bool> m_pauseFlag;
    std::atomic<int> m_totalFileCount;
    std::atomic<int> m_processedFileCount;
    std::atomic<int> m_activeThreadCount;
    QElapsedTimer m_totalTimer;
    std::mutex m_mutex; 
    QStringList m_failedFiles;
    std::mutex m_resizedFilesMutex;
    QStringList m_resizedFiles;
    std::mutex m_bucketDataMutex;
    QMap<int, qint64> m_bucketCompletionTimes;
    int m_bucketIdPadding;
    std::mutex m_taskMutex;
    std::condition_variable m_taskCond;
    QQueue<BatchStreamTask> m_readyTasks; 
    bool m_ioFinished = false; 
    QQueue<BatchStreamScannedFile> m_ssdTaskQueue;
    BatchStreamIoThreadMode m_ioMode = BatchStreamIoThreadMode::None;
    QThread* m_ioThread = nullptr;
    QThread* m_writerThread = nullptr;
    std::mutex m_writeMutex;
    std::condition_variable m_writeCond;
    QQueue<BatchStreamWriteTask> m_writeQueue;
    std::atomic<qint64> m_currentWriteBytes{0};
};
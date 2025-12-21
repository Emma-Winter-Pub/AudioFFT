#pragma once

#include "BatTypes.h"

#include <atomic>
#include <QObject>
#include <QElapsedTimer>


class BatWorker : public QObject {
    Q_OBJECT

public:
    explicit BatWorker(int bucketId, int bucketIdPadding, std::atomic<bool>* stopFlag, std::atomic<bool>* pauseFlag, QObject *parent = nullptr);

public slots:
    void processBucket(const Bucket& filesToProcess, const BatSettings& settings);

signals:
    void logMessage(const QString& message);
    void fileCompleted(const QString& filePath, bool success, const QString& errorInfo);
    void bucketFinished(int bucketId, qint64 elapsedMs);
    void fileMetricsReported(const FilePerformanceMetrics& metrics);

private:
    void checkPauseState();
    QString generateOutputFilePath(const QString& inputFile, const BatSettings& settings);

    int m_bucketId;
    int m_bucketIdPadding;
    std::atomic<bool>* m_stopFlag;
    std::atomic<bool>* m_pauseFlag;
    
    QElapsedTimer m_bucketTimer;
};
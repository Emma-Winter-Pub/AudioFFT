#pragma once

#include "BatTypes.h"
#include "BatProcessor.h" 

#include <QObject>
#include <QElapsedTimer>
#include <atomic>

class BatchStreamDecoder;
class BatchStreamFft;
class BatchStreamGenerator;

class BatWorker : public QObject {
    Q_OBJECT

public:
    explicit BatWorker(int bucketId, int bucketIdPadding, std::atomic<bool>* stopFlag, std::atomic<bool>* pauseFlag, BatProcessor* processor, QObject* parent = nullptr);
    void setProcessor(BatProcessor* processor) { m_processor = processor; }
    void setDirectWriteMode(bool direct) { m_directWriteMode = direct; }

public slots:
    void startWorkLoop(const BatSettings& settings);

signals:
    void logMessage(const QString& msg);
    void fileCompleted(const QString& filePath, bool success, const QString& errorMsg, const FilePerformanceMetrics& metrics, int threadId);
    void bucketFinished(int bucketId, qint64 elapsedMs);
    void fileMetricsReported(const FilePerformanceMetrics& metrics);

private:
    void checkPauseState();
    bool processSingleFile(BatTask& task, const BatSettings& settings, QString& errorMsg, bool& outIsResized, FilePerformanceMetrics& outMetrics);
    QString generateOutputFilePath(const QString& inputFile, const BatSettings& settings);
    int getRequiredFftSize(int height) const;
    int m_bucketId;
    int m_bucketIdPadding;
    std::atomic<bool>* m_stopFlag;
    std::atomic<bool>* m_pauseFlag;
    BatProcessor* m_processor = nullptr;
    QElapsedTimer m_bucketTimer;
    bool m_directWriteMode = true; 
};
#pragma once

#include "BatchStreamTypes.h"
#include "BatchStreamProcessor.h" 

#include <QObject>
#include <QList>
#include <QString>
#include <QElapsedTimer>
#include <atomic>

class BatchStreamDecoder;
class BatchStreamFft;
class BatchStreamGenerator;

class BatchStreamWorker : public QObject {
    Q_OBJECT

public:
    explicit BatchStreamWorker(int bucketId, int bucketIdPadding, std::atomic<bool>* stopFlag, std::atomic<bool>* pauseFlag, BatchStreamProcessor* processor, QObject* parent = nullptr);
    void setProcessor(BatchStreamProcessor* processor) { m_processor = processor; }
    void setDirectWriteMode(bool direct) { m_directWriteMode = direct; }

public slots:
    void startWorkLoop(const BatchStreamSettings& settings);

signals:
    void logMessage(const QString& msg);
    void fileFinished(const QString& filePath, bool success, const QString& errorMsg, int threadId, qint64 elapsedMs);
    void bucketFinished(int bucketId, qint64 elapsedMs);

private:
    void checkPause();
    bool processSingleFile(BatchStreamTask& task, const BatchStreamSettings& settings, BatchStreamFft& fft, BatchStreamGenerator& generator, QString& errorMsg, bool& outIsResized);
    QString generateOutputFilePath(const QString& inputFile, const BatchStreamSettings& settings);
    int getRequiredFftSize(int height) const;
    int m_bucketId;
    int m_bucketIdPadding;
    std::atomic<bool>* m_stopFlag;
    std::atomic<bool>* m_pauseFlag;
    BatchStreamProcessor* m_processor = nullptr;
    bool m_directWriteMode = true; 
};
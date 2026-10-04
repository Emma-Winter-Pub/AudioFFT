#pragma once

#include "BPBS_Types.h"
#include "BPBS_Cache.h"
#include "BPBS_MetaCache.h"
#include "BPBS_DemandQueue.h"

#include <QObject>
#include <QThreadPool>
#include <QThread>
#include <QTimer>
#include <QString>
#include <QList>
#include <deque>
#include <mutex>
#include <atomic>
#include <memory>
#include <condition_variable>

class BPBS_Worker;

class BPBS_Processor : public QObject {
    Q_OBJECT

public:
    explicit BPBS_Processor(QObject *parent = nullptr);
    ~BPBS_Processor() override;
    BPBS_Cache& cache() { return m_cache; }
    BPBS_MetaCache& metaCache() { return m_metaCache; }
    BPBS_DemandQueue& demandQueue() { return m_demandQueue; }
    void setFolderFileList(const QString& domainId, const QList<BPBS_DemandItem>& allFiles);
    void requestViewportUpdate(int firstRow, int lastRow);
    void updatePreviewSettings(const BPBS_Settings& settings);
    void updatePalette(const QString& paletteId, bool inverted, bool negative);
    BPBS_PreemptResult requestPreempt(const QString& filePath);
    void clearP1File();
    void freezeForExport();
    void unfreezeFromExport();
    BPBS_Settings currentSettings() const;
    QString currentDomain() const;
    void updateForegroundMetadata(const QString& filePath, const BPBS_AudioMetadata& meta);

public slots:
    void startPreviewWorkers();
    void pause();
    void resume();
    void stop();
    void setMetadataProbingPaused(bool visible);

signals:
    void logMessage(const QString& message);
    void thumbnailReady(const QString& filePath);
    void metadataReady(const QString& filePath, const BPBS_AudioMetadata& info);
    void fileNotAudio(const QString& filePath);
    void p1FileReleased(const QString& filePath);

private slots:
    void onDebounceTimeout();

private:
    void cleanupThreads();
    void runMetadataProbeLoop();
    void probeSingleFileMetadata(const QString& filePath);
    BPBS_Cache          m_cache;
    BPBS_MetaCache      m_metaCache;
    BPBS_DemandQueue    m_demandQueue;
    BPBS_CalcParams     m_currentCalcParams;
    BPBS_VisualParams   m_currentVisualParams;
    bool                m_syncMode = true;
    mutable std::mutex  m_settingsMutex;
    QString             m_currentDomain;
    std::atomic<bool>   m_isFastStorage{false};
    std::unique_ptr<QThreadPool> m_workerPool;
    std::atomic<bool>   m_stopFlag{false};
    std::atomic<bool>   m_pauseFlag{false};
    QTimer*             m_debounceTimer = nullptr;
    int                 m_pendingFirstRow = -1;
    int                 m_pendingLastRow = -1;
    void runIoReaderLoop();
    QThread*            m_ioReaderThread = nullptr;
    std::mutex          m_ioReaderMutex;
    std::condition_variable m_ioReaderCv;
    QThread*            m_metaProbeThread = nullptr;
    std::deque<QString> m_metaProbeQueue;
    std::mutex          m_metaProbeMutex;
    std::condition_variable m_metaProbeCv;
    std::atomic<bool>   m_metaProbePaused{false};
};
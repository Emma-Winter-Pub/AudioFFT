#pragma once

#include "BPBS_Types.h"
#include "BPBS_Cache.h"
#include "BPBS_MetaCache.h"
#include "BPBS_CancellationToken.h"

#include <QList>
#include <QHash>
#include <QString>
#include <vector>
#include <memory>
#include <mutex>
#include <condition_variable>
#include <atomic>
#include <optional>

class BPBS_DemandQueue {
public:
    explicit BPBS_DemandQueue(int maxWorkers);
    ~BPBS_DemandQueue();
    BPBS_DemandQueue(const BPBS_DemandQueue&) = delete;
    BPBS_DemandQueue& operator=(const BPBS_DemandQueue&) = delete;
    void setFolderFileList(const QList<BPBS_DemandItem>& allFiles);
    void updateViewportRange(int firstRow, int lastRow,
                             const BPBS_Cache& cache,
                             const BPBS_MetaCache& metaCache,
                             uint64_t currentFingerprint);
    void setFastStorage(bool isFast) noexcept;
    [[nodiscard]] bool isFastStorage() const noexcept;
    std::optional<BPBS_DemandItem> claimNextForReader();
    std::optional<BPBS_DemandItem> claim(int workerId,
                                         std::shared_ptr<BPBS_CancellationToken>& outToken,
                                         std::atomic<bool>* stopFlag,
                                         std::atomic<bool>* pauseFlag);
    void finishTask(int workerId, QString* outReleasedP1File = nullptr);
    void markCompleted(const QString& filePath);
    void setMemorySuspended(bool suspended);
    [[nodiscard]] bool isMemorySuspended() const noexcept;
    BPBS_PreemptResult preemptP1(const QString& filePath, bool isFastStorage);
    void clearP1File();
    bool isP1File(const QString& filePath) const;
    void clearAndAbortAll();
    void stop();
    void setPaused(bool paused);
    int calcDemandCount() const;
    int activeWorkerCount() const;
    int maxWorkers() const noexcept { return m_maxWorkers; }
    QList<BPBS_DemandItem> currentB() const;

private:
    size_t getSlotIndex(int workerId) const noexcept;
    bool hasClaimableTask_Locked() const noexcept;
    void reconcileSlots_Locked();
    mutable std::mutex m_mutex;
    std::condition_variable m_cv;
    const int m_maxWorkers;
    std::atomic<bool> m_stopped{false};
    std::atomic<bool> m_paused{false};
    std::atomic<bool> m_memorySuspended{false};
    std::atomic<bool> m_isFastStorage{false};
    QList<BPBS_DemandItem> m_folderFiles;
    QList<BPBS_DemandItem> m_B;
    QList<BPBS_DemandItem> m_Bcalc;
    QHash<QString, int> m_calcRankMap;
    QString m_p1FilePath;
    QString m_hddWaitingP1File;
    std::vector<BPBS_WorkerSlot> m_slots;
    int m_lastFirstRow = -1;
    int m_lastLastRow = -1;
};
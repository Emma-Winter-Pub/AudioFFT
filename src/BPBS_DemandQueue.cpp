#include "BPBS_DemandQueue.h"
#include "AudioCodecWeights.h"

#include <algorithm>
#include <cmath>

namespace {
    constexpr qint64 ONE_MB = 1024LL * 1024LL;
    double calculateCost(const BPBS_DemandItem& item) noexcept {
        double weight = CODEC_ID_WEIGHTS.value(static_cast<AVCodecID>(item.codecId), DEFAULT_CODEC_WEIGHT);
        if (item.durationMs > 0) {
            return (static_cast<double>(item.durationMs) / 1000.0) * weight;
        }
        return (static_cast<double>(item.fileSize) / 1048576.0) * weight;
    }
    bool compareCostWithSub1MB(const BPBS_DemandItem& a, const BPBS_DemandItem& b) noexcept {
        bool aSmall = (a.fileSize < ONE_MB);
        bool bSmall = (b.fileSize < ONE_MB);
        if (aSmall != bSmall) {
            return aSmall;
        }
        if (aSmall) {
            if (a.fileSize != b.fileSize) {
                return a.fileSize < b.fileSize;
            }
        } else {
            double costA = calculateCost(a);
            double costB = calculateCost(b);
            if (std::abs(costA - costB) > 1e-4) {
                return costA < costB;
            }
        }
        return a.filePath < b.filePath;
    }
    bool compareDistance(const BPBS_DemandItem& a, const BPBS_DemandItem& b) noexcept {
        if (a.distance != b.distance) {
            return a.distance < b.distance;
        }
        return a.filePath < b.filePath;
    }
}

BPBS_DemandQueue::BPBS_DemandQueue(int maxWorkers)
    : m_maxWorkers(std::max(1, maxWorkers))
    , m_slots(std::max(1, maxWorkers))
{
    for (int i = 0; i < m_maxWorkers; ++i) {
        m_slots[i].workerId = i + 1;
    }
}

BPBS_DemandQueue::~BPBS_DemandQueue() {
    stop();
}

size_t BPBS_DemandQueue::getSlotIndex(int workerId) const noexcept {
    if (workerId >= 1 && workerId <= m_maxWorkers) {
        return static_cast<size_t>(workerId - 1);
    }
    if (workerId >= 0 && workerId < m_maxWorkers) {
        return static_cast<size_t>(workerId);
    }
    return 0;
}

void BPBS_DemandQueue::setFolderFileList(const QList<BPBS_DemandItem>& allFiles) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_folderFiles = allFiles;
}

void BPBS_DemandQueue::updateViewportRange(int firstRow, int lastRow,
                                          const BPBS_Cache& cache,
                                          const BPBS_MetaCache& metaCache,
                                          uint64_t currentFingerprint)
{
    std::unique_lock<std::mutex> lock(m_mutex);
    m_memorySuspended.store(false, std::memory_order_relaxed);
    m_lastFirstRow = firstRow;
    m_lastLastRow = lastRow;
    if (m_folderFiles.isEmpty()) {
        m_B.clear();
        m_Bcalc.clear();
        m_calcRankMap.clear();
        reconcileSlots_Locked();
        return;
    }
    const int total = m_folderFiles.size();
    firstRow = std::max(0, std::min(firstRow, total - 1));
    lastRow = std::max(firstRow, std::min(lastRow, total - 1));
    QList<BPBS_DemandItem> p2;
    QList<BPBS_DemandItem> p3;
    QList<BPBS_DemandItem> p4;
    QList<BPBS_DemandItem> p5;
    p2.reserve(lastRow - firstRow + 1);
    p3.reserve(20);
    p4.reserve(10);
    p5.reserve(total);
    for (int i = firstRow; i <= lastRow; ++i) {
        p2.append(m_folderFiles[i]);
    }
    int p3End = std::min(total - 1, lastRow + 20);
    for (int i = lastRow + 1; i <= p3End; ++i) {
        p3.append(m_folderFiles[i]);
    }
    int p4Start = std::max(0, firstRow - 10);
    for (int i = p4Start; i < firstRow; ++i) {
        p4.append(m_folderFiles[i]);
    }
    for (int i = 0; i < p4Start; ++i) {
        BPBS_DemandItem item = m_folderFiles[i];
        item.distance = firstRow - i;
        p5.append(item);
    }
    for (int i = p3End + 1; i < total; ++i) {
        BPBS_DemandItem item = m_folderFiles[i];
        item.distance = i - lastRow;
        p5.append(item);
    }
    std::sort(p2.begin(), p2.end(), compareCostWithSub1MB);
    std::sort(p3.begin(), p3.end(), compareCostWithSub1MB);
    std::sort(p4.begin(), p4.end(), compareCostWithSub1MB);
    std::sort(p5.begin(), p5.end(), compareDistance);
    m_B.clear();
    m_B.reserve(total);
    m_B.append(p2);
    m_B.append(p3);
    m_B.append(p4);
    m_B.append(p5);
    for (int i = 0; i < m_B.size(); ++i) {
        m_B[i].rank = i + 1;
    }
    m_Bcalc.clear();
    m_Bcalc.reserve(m_B.size());
    m_calcRankMap.clear();
    int calcRank = 1;
    for (const auto& item : m_B) {
        if (metaCache.isKnownInvalid(item.filePath, item.fileSize)) {
            continue;
        }
        if (cache.hasValid(item.filePath, currentFingerprint)) {
            continue;
        }
        BPBS_DemandItem calcItem = item;
        calcItem.rank = calcRank;
        m_Bcalc.append(calcItem);
        m_calcRankMap.insert(calcItem.filePath, calcRank);
        calcRank++;
    }
    reconcileSlots_Locked();
    m_cv.notify_all();
}

void BPBS_DemandQueue::reconcileSlots_Locked() {
    for (auto& slot : m_slots) {
        if (!slot.isBusy()) continue;
        const QString& path = slot.filePath;
        if (!m_p1FilePath.isEmpty() && path == m_p1FilePath) {
            if (slot.token) slot.token->cancelTask();
            if (slot.buffer) slot.buffer->abort();
            continue;
        }
        auto it = m_calcRankMap.find(path);
        if (it == m_calcRankMap.end()) {
            if (slot.token) slot.token->cancelTask();
            if (slot.buffer) slot.buffer->abort();
            continue;
        }
        int rank = it.value();
        if (rank > m_maxWorkers) {
            if (slot.token) slot.token->cancelTask();
            if (slot.buffer) slot.buffer->abort();
        }
    }
}

bool BPBS_DemandQueue::hasClaimableTask_Locked() const noexcept {
    if (m_Bcalc.isEmpty()) return false;
    const bool isFast = m_isFastStorage.load(std::memory_order_relaxed);
    const int limit = std::min(m_maxWorkers, static_cast<int>(m_Bcalc.size()));
    for (int i = 0; i < limit; ++i) {
        const auto& item = m_Bcalc[i];
        if (!m_p1FilePath.isEmpty() && item.filePath == m_p1FilePath) {
            continue;
        }
        bool running = false;
        for (const auto& slot : m_slots) {
            if (slot.isBusy() && slot.filePath == item.filePath) {
                running = true;
                break;
            }
        }
        if (!running) {
            if (isFast || item.buffer != nullptr) {
                return true;
            }
        }
    }
    return false;
}

void BPBS_DemandQueue::assignBufferAndCodec(const QString& filePath, std::shared_ptr<BPBS_Buffer> buffer, int codecId) {
    std::lock_guard<std::mutex> lock(m_mutex);
    for (auto& item : m_Bcalc) {
        if (item.filePath == filePath) {
            item.buffer = buffer;
            item.codecId = codecId;
            break;
        }
    }
    m_cv.notify_all();
}

std::optional<BPBS_DemandItem> BPBS_DemandQueue::claim(
    int workerId,
    std::shared_ptr<BPBS_CancellationToken>& outToken,
    std::atomic<bool>* stopFlag,
    std::atomic<bool>* pauseFlag)
{
    std::unique_lock<std::mutex> lock(m_mutex);
    m_cv.wait(lock, [this, stopFlag, pauseFlag]() {
        if ((stopFlag && stopFlag->load(std::memory_order_relaxed)) || m_stopped.load(std::memory_order_relaxed)) {
            return true;
        }
        if ((pauseFlag && pauseFlag->load(std::memory_order_relaxed)) || m_paused.load(std::memory_order_relaxed)) {
            return false;
        }
        if (m_memorySuspended.load(std::memory_order_relaxed)) {
            return false;
        }
        return hasClaimableTask_Locked();
    });
    if ((stopFlag && stopFlag->load(std::memory_order_relaxed)) || m_stopped.load(std::memory_order_relaxed)) {
        return std::nullopt;
    }
    const int limit = std::min(m_maxWorkers, static_cast<int>(m_Bcalc.size()));
    for (int i = 0; i < limit; ++i) {
        const auto& item = m_Bcalc[i];
        if (!m_p1FilePath.isEmpty() && item.filePath == m_p1FilePath) {
            continue;
        }
        bool running = false;
        for (const auto& slot : m_slots) {
            if (slot.isBusy() && slot.filePath == item.filePath) {
                running = true;
                break;
            }
        }
        const bool isFast = m_isFastStorage.load(std::memory_order_relaxed);
        if (!running && (isFast || item.buffer != nullptr)) {
            size_t slotIdx = getSlotIndex(workerId);
            auto& mySlot = m_slots[slotIdx];
            mySlot.workerId = workerId;
            mySlot.filePath = item.filePath;
            mySlot.buffer = item.buffer;
            mySlot.token = std::make_shared<BPBS_CancellationToken>();
            mySlot.token->setProcessorStopFlag(stopFlag);
            outToken = mySlot.token;
            return item;
        }
    }
    return std::nullopt;
}

void BPBS_DemandQueue::finishTask(int workerId, QString* outReleasedP1File) {
    std::lock_guard<std::mutex> lock(m_mutex);
    size_t slotIdx = getSlotIndex(workerId);
    if (slotIdx < m_slots.size()) {
        auto& slot = m_slots[slotIdx];
        if (!m_hddWaitingP1File.isEmpty() && slot.filePath == m_hddWaitingP1File) {
            if (outReleasedP1File) {
                *outReleasedP1File = m_hddWaitingP1File;
            }
            m_hddWaitingP1File.clear();
        }
        slot.clear();
    }
    m_cv.notify_one();
}

void BPBS_DemandQueue::markCompleted(const QString& filePath) {
    std::lock_guard<std::mutex> lock(m_mutex);
    bool removed = false;
    for (int i = 0; i < m_Bcalc.size(); ++i) {
        if (m_Bcalc[i].filePath == filePath) {
            m_Bcalc.removeAt(i);
            removed = true;
            break;
        }
    }
    if (removed) {
        m_calcRankMap.remove(filePath);
        for (int i = 0; i < m_Bcalc.size(); ++i) {
            m_Bcalc[i].rank = i + 1;
            m_calcRankMap[m_Bcalc[i].filePath] = i + 1;
        }
        reconcileSlots_Locked();
        m_cv.notify_all();
    }
}

void BPBS_DemandQueue::setMemorySuspended(bool suspended) {
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_memorySuspended.store(suspended, std::memory_order_relaxed);
    }
    if (!suspended) {
        m_cv.notify_all();
    }
}

bool BPBS_DemandQueue::isMemorySuspended() const noexcept {
    return m_memorySuspended.load(std::memory_order_relaxed);
}

BPBS_PreemptResult BPBS_DemandQueue::preemptP1(const QString& filePath, bool isFastStorage) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_p1FilePath = filePath;
    bool foundRunning = false;
    for (auto& slot : m_slots) {
        if (slot.isBusy() && slot.filePath == filePath) {
            foundRunning = true;
            if (slot.token) {
                slot.token->cancelTask();
            }
            break;
        }
    }
    if (!foundRunning) {
        return BPBS_PreemptResult::NotOwned;
    }
    if (isFastStorage) {
        return BPBS_PreemptResult::Preempted_SSD;
    } else {
        m_hddWaitingP1File = filePath;
        return BPBS_PreemptResult::Preempted_HDD;
    }
}

void BPBS_DemandQueue::clearP1File() {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_p1FilePath.clear();
    m_hddWaitingP1File.clear();
    m_cv.notify_all();
}

bool BPBS_DemandQueue::isP1File(const QString& filePath) const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return !m_p1FilePath.isEmpty() && m_p1FilePath == filePath;
}

void BPBS_DemandQueue::clearAndAbortAll() {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_memorySuspended.store(false, std::memory_order_relaxed);
    m_folderFiles.clear();
    m_B.clear();
    m_Bcalc.clear();
    m_calcRankMap.clear();
    m_p1FilePath.clear();
    m_hddWaitingP1File.clear();
    m_lastFirstRow = -1;
    m_lastLastRow = -1;
    for (auto& slot : m_slots) {
        if (slot.isBusy() && slot.token) {
            slot.token->cancelTask();
        }
        slot.clear();
    }
    m_cv.notify_all();
}

void BPBS_DemandQueue::stop() {
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_stopped.store(true, std::memory_order_relaxed);
        for (auto& slot : m_slots) {
            if (slot.isBusy() && slot.token) {
                slot.token->cancelTask();
            }
        }
    }
    m_cv.notify_all();
}

void BPBS_DemandQueue::setPaused(bool paused) {
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_paused.store(paused, std::memory_order_relaxed);
    }
    if (!paused) {
        m_cv.notify_all();
    }
}

int BPBS_DemandQueue::calcDemandCount() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_Bcalc.size();
}

int BPBS_DemandQueue::activeWorkerCount() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    int count = 0;
    for (const auto& slot : m_slots) {
        if (slot.isBusy()) count++;
    }
    return count;
}

QList<BPBS_DemandItem> BPBS_DemandQueue::currentB() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_B;
}

void BPBS_DemandQueue::setFastStorage(bool isFast) noexcept {
    m_isFastStorage.store(isFast, std::memory_order_relaxed);
    m_cv.notify_all();
}

bool BPBS_DemandQueue::isFastStorage() const noexcept {
    return m_isFastStorage.load(std::memory_order_relaxed);
}

std::optional<BPBS_DemandItem> BPBS_DemandQueue::claimNextForReader() {
    std::lock_guard<std::mutex> lock(m_mutex);
    if (m_isFastStorage.load(std::memory_order_relaxed) ||
        m_stopped.load(std::memory_order_relaxed) ||
        m_paused.load(std::memory_order_relaxed))
    {
        return std::nullopt;
    }
    const int limit = std::min(m_maxWorkers, static_cast<int>(m_Bcalc.size()));
    for (int i = 0; i < limit; ++i) {
        auto& item = m_Bcalc[i];
        if (!m_p1FilePath.isEmpty() && item.filePath == m_p1FilePath) {
            continue;
        }
        if (!item.buffer && !item.isReaderAssigned) {
            item.isReaderAssigned = true;
            return item;
        }
    }
    return std::nullopt;
}
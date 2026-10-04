#pragma once

#include "BPBS_Asset.h"
#include "BPBS_Types.h"

#include <QString>
#include <QStringList>
#include <QHash>
#include <QList>
#include <vector>
#include <memory>
#include <variant>
#include <shared_mutex>
#include <cstdint>

inline constexpr qint64 BPBS_INLINE_ASSET_THRESHOLD = 256LL * 1024;
inline constexpr qint64 BPBS_DEFAULT_CACHE_MAX_BYTES = 512LL * 1024 * 1024;

struct BPBS_FileEntry {
    bool isInline = false;
    std::variant<BPBS_Asset, BPBS_AssetPtr> storage;
    BPBS_Asset& asset() noexcept {
        if (isInline) {
            return std::get<BPBS_Asset>(storage);
        }
        return *std::get<BPBS_AssetPtr>(storage);
    }
    const BPBS_Asset& asset() const noexcept {
        if (isInline) {
            return std::get<BPBS_Asset>(storage);
        }
        return *std::get<BPBS_AssetPtr>(storage);
    }
    BPBS_AssetPtr toSharedPtr() const {
        if (isInline) {
            return std::make_shared<BPBS_Asset>(std::get<BPBS_Asset>(storage));
        }
        return std::get<BPBS_AssetPtr>(storage);
    }
};

struct BPBS_DomainBucket {
    QString domainId;
    qint64 leaveTime = 0;
    bool isCurrent = false;
    qint64 domainBytes = 0;
    QHash<QString, BPBS_FileEntry> fileTable;
};

class BPBS_Cache {
public:
    explicit BPBS_Cache(qint64 maxMemoryBytes = BPBS_DEFAULT_CACHE_MAX_BYTES);
    ~BPBS_Cache() = default;
    BPBS_Cache(const BPBS_Cache&) = delete;
    BPBS_Cache& operator=(const BPBS_Cache&) = delete;
    bool hasValid(const QString& domainId, const QString& fileName, uint64_t fingerprint) const;
    bool hasValid(const QString& filePath, uint64_t fingerprint) const;
    BPBS_AssetPtr find(const QString& domainId, const QString& fileName, uint64_t fingerprint = 0) const;
    BPBS_AssetPtr find(const QString& filePath, uint64_t fingerprint = 0) const;
    bool insert(BPBS_AssetPtr asset, const QList<BPBS_DemandItem>& currentB, bool& outShouldSuspend);
    bool insert(BPBS_AssetPtr asset, const QList<BPBS_DemandItem>& currentB) {
        bool dummy = false;
        return insert(asset, currentB, dummy);
    }
    void setCurrentDomain(const QString& domainId);
    void onDomainLeave(const QString& oldDomainId, qint64 leaveTime, const QList<BPBS_DemandItem>& lastB);
    void updateAllColorTables(const QList<QRgb>& colors);
    void remove(const QString& filePath);
    void clear();
    qint64 memoryUsage() const;
    qint64 maxMemoryCapacity() const;
    bool isFull() const;
    QStringList getAllPaths() const;
    std::vector<BPBS_AssetPtr> popAllReady(uint64_t fingerprint);

private:
    static void splitPath(const QString& fullPath, QString& outDomain, QString& outFileName);
    void evictHistoricalDomains_Locked(qint64& bytesNeeded);
    void evictActiveDomainTail_Locked(qint64& bytesNeeded, const QList<BPBS_DemandItem>& currentB, int incomingRank);
    mutable std::shared_mutex m_mutex;
    qint64 m_maxBytes;
    qint64 m_currentTotalBytes = 0;
    QHash<QString, std::shared_ptr<BPBS_DomainBucket>> m_domains;
    std::shared_ptr<BPBS_DomainBucket> m_activeDomain;
};
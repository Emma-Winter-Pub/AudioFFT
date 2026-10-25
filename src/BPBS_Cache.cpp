#include "BPBS_Cache.h"

#include <QFileInfo>
#include <QDir>
#include <algorithm>
#include <limits>

namespace {
    QString normalizeDirPath(const QString& path) {
        if (path.isEmpty()) return QString();
        QString clean = QDir::cleanPath(path);
        clean.replace('\\', '/');
        return clean;
    }
}

BPBS_Cache::BPBS_Cache(qint64 maxMemoryBytes)
    : m_maxBytes(maxMemoryBytes)
    , m_currentTotalBytes(0)
{
}

void BPBS_Cache::splitPath(const QString& fullPath, QString& outDomain, QString& outFileName) {
    QString clean = normalizeDirPath(fullPath);
    int lastSlash = clean.lastIndexOf('/');
    if (lastSlash != -1) {
        outDomain = clean.left(lastSlash);
        outFileName = clean.mid(lastSlash + 1);
    } else {
        outDomain.clear();
        outFileName = clean;
    }
}

bool BPBS_Cache::hasValid(const QString& domainId, const QString& fileName, uint64_t fingerprint) const {
    std::shared_lock<std::shared_mutex> lock(m_mutex);
    const BPBS_DomainBucket* targetBucket = nullptr;
    if (m_activeDomain && m_activeDomain->domainId == domainId) {
        targetBucket = m_activeDomain.get();
    } else {
        auto it = m_domains.find(domainId);
        if (it != m_domains.end()) {
            targetBucket = it.value().get();
        }
    }
    if (!targetBucket) {
        return false;
    }
    auto fit = targetBucket->fileTable.find(fileName);
    if (fit == targetBucket->fileTable.end()) {
        return false;
    }
    const BPBS_Asset& a = fit.value().asset();
    return (a.fingerprint == fingerprint && a.hasL0());
}

bool BPBS_Cache::hasValid(const QString& filePath, uint64_t fingerprint) const {
    QString domain, fileName;
    splitPath(filePath, domain, fileName);
    return hasValid(domain, fileName, fingerprint);
}

BPBS_AssetPtr BPBS_Cache::find(const QString& domainId, const QString& fileName, uint64_t fingerprint) const {
    std::shared_lock<std::shared_mutex> lock(m_mutex);
    const BPBS_DomainBucket* targetBucket = nullptr;
    if (m_activeDomain && m_activeDomain->domainId == domainId) {
        targetBucket = m_activeDomain.get();
    } else {
        auto it = m_domains.find(domainId);
        if (it != m_domains.end()) {
            targetBucket = it.value().get();
        }
    }
    if (!targetBucket) {
        return nullptr;
    }
    auto fit = targetBucket->fileTable.find(fileName);
    if (fit == targetBucket->fileTable.end()) {
        return nullptr;
    }
    const BPBS_FileEntry& entry = fit.value();
    const BPBS_Asset& a = entry.asset();
    if (fingerprint != 0 && a.fingerprint != fingerprint) {
        return nullptr;
    }
    return entry.toSharedPtr();
}

BPBS_AssetPtr BPBS_Cache::find(const QString& filePath, uint64_t fingerprint) const {
    QString domain, fileName;
    splitPath(filePath, domain, fileName);
    return find(domain, fileName, fingerprint);
}

bool BPBS_Cache::insert(BPBS_AssetPtr asset, const QList<BPBS_DemandItem>& currentB, bool& outShouldSuspend) {
    outShouldSuspend = false;
    if (!asset || asset->filePath.isEmpty()) return false;
    QString domain = asset->domainId;
    QString fileName;
    if (domain.isEmpty()) {
        splitPath(asset->filePath, domain, fileName);
        asset->domainId = domain;
    } else {
        domain = normalizeDirPath(domain);
        splitPath(asset->filePath, domain, fileName);
    }
    qint64 newAssetBytes = asset->estimatedMemoryUsage();
    int incomingRank = std::numeric_limits<int>::max();
    for (const auto& item : currentB) {
        if (item.filePath == asset->filePath) {
            incomingRank = item.rank;
            break;
        }
    }
    std::unique_lock<std::shared_mutex> lock(m_mutex);
    std::shared_ptr<BPBS_DomainBucket> bucket;
    auto dit = m_domains.find(domain);
    if (dit != m_domains.end()) {
        bucket = dit.value();
    } else {
        bucket = std::make_shared<BPBS_DomainBucket>();
        bucket->domainId = domain;
        bucket->isCurrent = (m_activeDomain && m_activeDomain->domainId == domain);
        bucket->leaveTime = bucket->isCurrent ? 0 : 1;
        m_domains.insert(domain, bucket);
    }
    auto fit = bucket->fileTable.find(fileName);
    if (fit != bucket->fileTable.end()) {
        qint64 oldBytes = fit.value().asset().estimatedMemoryUsage();
        bucket->domainBytes -= oldBytes;
        m_currentTotalBytes -= oldBytes;
        bucket->fileTable.erase(fit);
    }
    if (m_currentTotalBytes + newAssetBytes > m_maxBytes) {
        qint64 bytesNeeded = (m_currentTotalBytes + newAssetBytes) - m_maxBytes;
        evictHistoricalDomains_Locked(bytesNeeded);
        if (m_currentTotalBytes + newAssetBytes > m_maxBytes) {
            int worstRankInCache = -1;
            QHash<QString, int> rankMap;
            rankMap.reserve(currentB.size());
            for (const auto& item : currentB) {
                QString d, fn;
                splitPath(item.filePath, d, fn);
                rankMap.insert(fn, item.rank);
            }
            if (m_activeDomain) {
                for (auto it = m_activeDomain->fileTable.begin(); it != m_activeDomain->fileTable.end(); ++it) {
                    int r = rankMap.value(it.key(), std::numeric_limits<int>::max());
                    if (r > worstRankInCache) {
                        worstRankInCache = r;
                    }
                }
            }
            if (worstRankInCache != -1 && incomingRank > worstRankInCache) {
                outShouldSuspend = true;
                return false;
            }
            bytesNeeded = (m_currentTotalBytes + newAssetBytes) - m_maxBytes;
            evictActiveDomainTail_Locked(bytesNeeded, currentB, incomingRank);
            if (m_currentTotalBytes + newAssetBytes > m_maxBytes) {
                outShouldSuspend = true;
                return false;
            }
        }
    }
    BPBS_FileEntry entry;
    if (newAssetBytes <= BPBS_INLINE_ASSET_THRESHOLD) {
        entry.isInline = true;
        entry.storage = *asset;
    } else {
        entry.isInline = false;
        entry.storage = asset;
    }
    bucket->fileTable.insert(fileName, std::move(entry));
    bucket->domainBytes += newAssetBytes;
    m_currentTotalBytes += newAssetBytes;
    return true;
}

void BPBS_Cache::setCurrentDomain(const QString& domainId) {
    QString cleanDomain = normalizeDirPath(domainId);
    if (cleanDomain.isEmpty()) return;
    std::unique_lock<std::shared_mutex> lock(m_mutex);
    auto it = m_domains.find(cleanDomain);
    if (it != m_domains.end()) {
        m_activeDomain = it.value();
        m_activeDomain->isCurrent = true;
        m_activeDomain->leaveTime = 0;
    } else {
        auto bucket = std::make_shared<BPBS_DomainBucket>();
        bucket->domainId = cleanDomain;
        bucket->isCurrent = true;
        bucket->leaveTime = 0;
        m_domains.insert(cleanDomain, bucket);
        m_activeDomain = bucket;
    }
}

void BPBS_Cache::onDomainLeave(const QString& oldDomainId, qint64 leaveTime, const QList<BPBS_DemandItem>& lastB) {
    QString cleanDomain = normalizeDirPath(oldDomainId);
    if (cleanDomain.isEmpty()) return;
    std::unique_lock<std::shared_mutex> lock(m_mutex);
    auto dit = m_domains.find(cleanDomain);
    if (dit == m_domains.end()) return;
    std::shared_ptr<BPBS_DomainBucket> bucket = dit.value();
    bucket->isCurrent = false;
    bucket->leaveTime = (leaveTime > 0) ? leaveTime : 1;
    if (m_activeDomain && m_activeDomain->domainId == cleanDomain) {
        m_activeDomain.reset();
    }
    QHash<QString, int> rankMap;
    rankMap.reserve(lastB.size());
    for (const auto& item : lastB) {
        QString itemDomain, itemFileName;
        splitPath(item.filePath, itemDomain, itemFileName);
        rankMap.insert(itemFileName, item.rank);
    }
    for (auto it = bucket->fileTable.begin(); it != bucket->fileTable.end(); ++it) {
        const QString& fileName = it.key();
        BPBS_Asset& a = it.value().asset();
        if (rankMap.contains(fileName)) {
            a.lastKnownRank = rankMap.value(fileName);
        } else {
            a.lastKnownRank = std::numeric_limits<int>::max();
        }
    }
}

void BPBS_Cache::updateAllColorTables(const QList<QRgb>& colors) {
    std::unique_lock<std::shared_mutex> lock(m_mutex);
    for (auto dit = m_domains.begin(); dit != m_domains.end(); ++dit) {
        auto& bucket = dit.value();
        for (auto fit = bucket->fileTable.begin(); fit != bucket->fileTable.end(); ++fit) {
            fit.value().asset().updateColorTable(colors);
        }
    }
}

void BPBS_Cache::remove(const QString& filePath) {
    QString domain, fileName;
    splitPath(filePath, domain, fileName);
    std::unique_lock<std::shared_mutex> lock(m_mutex);
    auto dit = m_domains.find(domain);
    if (dit == m_domains.end()) return;
    auto& bucket = dit.value();
    auto fit = bucket->fileTable.find(fileName);
    if (fit != bucket->fileTable.end()) {
        qint64 bytes = fit.value().asset().estimatedMemoryUsage();
        bucket->domainBytes -= bytes;
        m_currentTotalBytes -= bytes;
        bucket->fileTable.erase(fit);
    }
}

void BPBS_Cache::clear() {
    std::unique_lock<std::shared_mutex> lock(m_mutex);
    m_domains.clear();
    m_activeDomain.reset();
    m_currentTotalBytes = 0;
}

qint64 BPBS_Cache::memoryUsage() const {
    std::shared_lock<std::shared_mutex> lock(m_mutex);
    return m_currentTotalBytes;
}

qint64 BPBS_Cache::maxMemoryCapacity() const {
    return m_maxBytes;
}

bool BPBS_Cache::isFull() const {
    std::shared_lock<std::shared_mutex> lock(m_mutex);
    return m_currentTotalBytes >= m_maxBytes;
}

QStringList BPBS_Cache::getAllPaths() const {
    std::shared_lock<std::shared_mutex> lock(m_mutex);
    QStringList paths;
    for (auto dit = m_domains.begin(); dit != m_domains.end(); ++dit) {
        const auto& bucket = dit.value();
        for (auto fit = bucket->fileTable.begin(); fit != bucket->fileTable.end(); ++fit) {
            paths.append(fit.value().asset().filePath);
        }
    }
    return paths;
}

std::vector<BPBS_AssetPtr> BPBS_Cache::popAllReady(uint64_t fingerprint) {
    std::unique_lock<std::shared_mutex> lock(m_mutex);
    std::vector<BPBS_AssetPtr> result;
    for (auto dit = m_domains.begin(); dit != m_domains.end(); ++dit) {
        auto& bucket = dit.value();
        for (auto fit = bucket->fileTable.begin(); fit != bucket->fileTable.end(); ) {
            const BPBS_Asset& a = fit.value().asset();
            if (a.fingerprint == fingerprint && a.hasL0()) {
                qint64 bytes = a.estimatedMemoryUsage();
                result.push_back(fit.value().toSharedPtr());
                bucket->domainBytes -= bytes;
                m_currentTotalBytes -= bytes;
                fit = bucket->fileTable.erase(fit);
            } else {
                ++fit;
            }
        }
    }
    return result;
}

void BPBS_Cache::evictHistoricalDomains_Locked(qint64& bytesNeeded) {
    if (bytesNeeded <= 0) return;
    std::vector<std::shared_ptr<BPBS_DomainBucket>> historicalBuckets;
    for (auto it = m_domains.begin(); it != m_domains.end(); ++it) {
        if (!it.value()->isCurrent) {
            historicalBuckets.push_back(it.value());
        }
    }
    std::sort(historicalBuckets.begin(), historicalBuckets.end(),
              [](const std::shared_ptr<BPBS_DomainBucket>& a, const std::shared_ptr<BPBS_DomainBucket>& b) {
                  return a->leaveTime < b->leaveTime;
              });
    for (auto& bucket : historicalBuckets) {
        if (bytesNeeded <= 0) return;
        std::vector<QString> l0Candidates;
        for (auto fit = bucket->fileTable.begin(); fit != bucket->fileTable.end(); ++fit) {
            if (fit.value().asset().hasL0()) {
                l0Candidates.push_back(fit.key());
            }
        }
        std::sort(l0Candidates.begin(), l0Candidates.end(), [&bucket](const QString& a, const QString& b) {
            return bucket->fileTable[a].asset().lastKnownRank > bucket->fileTable[b].asset().lastKnownRank;
        });
        for (const QString& fileName : l0Candidates) {
            auto fit = bucket->fileTable.find(fileName);
            if (fit == bucket->fileTable.end()) continue;
            BPBS_Asset& asset = fit.value().asset();
            qint64 before = asset.estimatedMemoryUsage();
            asset.dropL0();
            qint64 diff = before - asset.estimatedMemoryUsage();
            bucket->domainBytes -= diff;
            m_currentTotalBytes -= diff;
            bytesNeeded -= diff;
            if (bytesNeeded <= 0) return;
        }
        std::vector<QString> eraseCandidates;
        for (auto fit = bucket->fileTable.begin(); fit != bucket->fileTable.end(); ++fit) {
            eraseCandidates.push_back(fit.key());
        }
        std::sort(eraseCandidates.begin(), eraseCandidates.end(), [&bucket](const QString& a, const QString& b) {
            return bucket->fileTable[a].asset().lastKnownRank > bucket->fileTable[b].asset().lastKnownRank;
        });
        for (const QString& fileName : eraseCandidates) {
            auto fit = bucket->fileTable.find(fileName);
            if (fit == bucket->fileTable.end()) continue;
            qint64 bytes = fit.value().asset().estimatedMemoryUsage();
            bucket->domainBytes -= bytes;
            m_currentTotalBytes -= bytes;
            bytesNeeded -= bytes;
            bucket->fileTable.erase(fit);
            if (bytesNeeded <= 0) return;
        }
        if (bucket->fileTable.isEmpty()) {
            m_domains.remove(bucket->domainId);
        }
    }
}

void BPBS_Cache::evictActiveDomainTail_Locked(qint64& bytesNeeded, const QList<BPBS_DemandItem>& currentB, int incomingRank) {
    if (bytesNeeded <= 0 || !m_activeDomain || m_activeDomain->fileTable.isEmpty()) return;
    QHash<QString, int> rankMap;
    rankMap.reserve(currentB.size());
    for (const auto& item : currentB) {
        QString itemDomain, itemFileName;
        splitPath(item.filePath, itemDomain, itemFileName);
        rankMap.insert(itemFileName, item.rank);
    }
    auto getCurrRank = [&rankMap](const QString& fileName) -> int {
        return rankMap.value(fileName, std::numeric_limits<int>::max());
    };
    std::vector<QString> currL0Candidates;
    for (auto fit = m_activeDomain->fileTable.begin(); fit != m_activeDomain->fileTable.end(); ++fit) {
        if (fit.value().asset().hasL0() && getCurrRank(fit.key()) > incomingRank) {
            currL0Candidates.push_back(fit.key());
        }
    }
    std::sort(currL0Candidates.begin(), currL0Candidates.end(), [&](const QString& a, const QString& b) {
        return getCurrRank(a) > getCurrRank(b);
    });
    for (const QString& fileName : currL0Candidates) {
        auto fit = m_activeDomain->fileTable.find(fileName);
        if (fit == m_activeDomain->fileTable.end()) continue;
        BPBS_Asset& asset = fit.value().asset();
        qint64 before = asset.estimatedMemoryUsage();
        asset.dropL0();
        qint64 diff = before - asset.estimatedMemoryUsage();
        m_activeDomain->domainBytes -= diff;
        m_currentTotalBytes -= diff;
        bytesNeeded -= diff;
        if (bytesNeeded <= 0) return;
    }
    std::vector<QString> currEraseCandidates;
    for (auto fit = m_activeDomain->fileTable.begin(); fit != m_activeDomain->fileTable.end(); ++fit) {
        if (getCurrRank(fit.key()) > incomingRank) {
            currEraseCandidates.push_back(fit.key());
        }
    }
    std::sort(currEraseCandidates.begin(), currEraseCandidates.end(), [&](const QString& a, const QString& b) {
        return getCurrRank(a) > getCurrRank(b);
    });
    for (const QString& fileName : currEraseCandidates) {
        auto fit = m_activeDomain->fileTable.find(fileName);
        if (fit == m_activeDomain->fileTable.end()) continue;
        qint64 bytes = fit.value().asset().estimatedMemoryUsage();
        m_activeDomain->domainBytes -= bytes;
        m_currentTotalBytes -= bytes;
        bytesNeeded -= bytes;
        m_activeDomain->fileTable.erase(fit);
        if (bytesNeeded <= 0) return;
    }
}
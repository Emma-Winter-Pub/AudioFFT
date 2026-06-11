#include "StorageCache.h"

StorageCache& StorageCache::instance() {
    static StorageCache s_instance;
    return s_instance;
}

bool StorageCache::tryGet(const QString& volumeKey, VolumeTopology& outTopology) const {
    if (volumeKey.isEmpty()) {
        return false;
    }
    std::shared_lock<std::shared_mutex> lock(m_mutex);
    auto it = m_cache.find(volumeKey);
    if (it != m_cache.end()) {
        if (it.value().deadline.hasExpired()) {
            return false;
        }
        outTopology = it.value().topology;
        return true;
    }
    return false;
}

void StorageCache::insert(const QString& volumeKey, const VolumeTopology& topology) {
    if (volumeKey.isEmpty()) {
        return;
    }
    qint64 ttlMs = 15000;
    if (topology.hasUSB || topology.isNetworkStorage || topology.domain == StorageDomain::NetworkFileSystem) {
        ttlMs = 2000;
    }
    else if (topology.isStorageSpaces || topology.isRaid) {
        ttlMs = 5000;
    }
    CacheEntry entry;
    entry.topology = topology;
    entry.deadline.setRemainingTime(ttlMs);
    std::unique_lock<std::shared_mutex> lock(m_mutex);
    m_cache.insert(volumeKey, entry);
}

void StorageCache::clear() {
    std::unique_lock<std::shared_mutex> lock(m_mutex);
    m_cache.clear();
}
#pragma once

#include "StorageTypes.h"

#include <QString>
#include <QHash>
#include <shared_mutex>
#include <QDeadlineTimer>

class StorageCache {
public:
    static StorageCache& instance();
    bool tryGet(const QString& volumeKey, VolumeTopology& outTopology) const;
    void insert(const QString& volumeKey, const VolumeTopology& topology);
    void clear();

private:
    StorageCache() = default;
    ~StorageCache() = default;
    StorageCache(const StorageCache&) = delete;
    StorageCache& operator=(const StorageCache&) = delete;
    struct CacheEntry {
        VolumeTopology topology;
        QDeadlineTimer deadline;
    };
    QHash<QString, CacheEntry> m_cache;
    mutable std::shared_mutex m_mutex;
};
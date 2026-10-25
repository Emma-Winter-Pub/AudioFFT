#include "BPBS_MetaCache.h"

#include <QDir>
#include <algorithm>

namespace {
    QString normalizeDirPath(const QString& path) {
        if (path.isEmpty()) return QString();
        QString clean = QDir::cleanPath(path);
        clean.replace('\\', '/');
        return clean;
    }
}

void BPBS_MetaCache::splitPath(const QString& fullPath, QString& outDomain, QString& outFileName) {
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

void BPBS_MetaCache::setCurrentDomain(const QString& domainId) {
    QString cleanDomain = normalizeDirPath(domainId);
    if (cleanDomain.isEmpty()) return;
    std::unique_lock<std::shared_mutex> lock(m_mutex);
    auto it = m_domains.find(cleanDomain);
    if (it != m_domains.end()) {
        m_activeDomain = it.value();
    } else {
        auto bucket = std::make_shared<BPBS_DomainMetaBucket>();
        bucket->domainId = cleanDomain;
        m_domains.insert(cleanDomain, bucket);
        m_activeDomain = bucket;
    }
}

MetadataPriority BPBS_MetaCache::getPriority(const QString& domainId, const QString& fileName, qint64 currentFileSize) const {
    std::shared_lock<std::shared_mutex> lock(m_mutex);
    const BPBS_DomainMetaBucket* targetBucket = nullptr;
    if (m_activeDomain && m_activeDomain->domainId == domainId) {
        targetBucket = m_activeDomain.get();
    } else {
        auto it = m_domains.find(domainId);
        if (it != m_domains.end()) {
            targetBucket = it.value().get();
        }
    }
    if (!targetBucket) {
        return MetadataPriority::Unprobed;
    }
    auto fit = targetBucket->fileTable.find(fileName);
    if (fit == targetBucket->fileTable.end()) {
        return MetadataPriority::Unprobed;
    }
    const BPBS_MetaRecord& r = fit.value();
    if (currentFileSize > 0 && r.fileSize != currentFileSize) {
        return MetadataPriority::Unprobed;
    }
    return r.priority;
}

MetadataPriority BPBS_MetaCache::getPriority(const QString& filePath, qint64 currentFileSize) const {
    QString domain, fileName;
    splitPath(filePath, domain, fileName);
    return getPriority(domain, fileName, currentFileSize);
}

bool BPBS_MetaCache::isKnownInvalid(const QString& domainId, const QString& fileName, qint64 currentFileSize) const {
    return getPriority(domainId, fileName, currentFileSize) == MetadataPriority::InvalidAudio;
}

bool BPBS_MetaCache::isKnownInvalid(const QString& filePath, qint64 currentFileSize) const {
    QString domain, fileName;
    splitPath(filePath, domain, fileName);
    return isKnownInvalid(domain, fileName, currentFileSize);
}

bool BPBS_MetaCache::query(const QString& domainId, const QString& fileName, qint64 currentFileSize,
                           BPBS_AudioMetadata& outMeta, MetadataPriority& outPriority) const
{
    std::shared_lock<std::shared_mutex> lock(m_mutex);
    const BPBS_DomainMetaBucket* targetBucket = nullptr;
    if (m_activeDomain && m_activeDomain->domainId == domainId) {
        targetBucket = m_activeDomain.get();
    } else {
        auto it = m_domains.find(domainId);
        if (it != m_domains.end()) {
            targetBucket = it.value().get();
        }
    }
    if (!targetBucket) {
        outPriority = MetadataPriority::Unprobed;
        return false;
    }
    auto fit = targetBucket->fileTable.find(fileName);
    if (fit == targetBucket->fileTable.end()) {
        outPriority = MetadataPriority::Unprobed;
        return false;
    }
    const BPBS_MetaRecord& r = fit.value();
    if (currentFileSize > 0 && r.fileSize != currentFileSize) {
        outPriority = MetadataPriority::Unprobed;
        return false;
    }
    outPriority = r.priority;
    outMeta = r.metadata;
    return (r.priority != MetadataPriority::Unprobed);
}

bool BPBS_MetaCache::query(const QString& filePath, qint64 currentFileSize,
                           BPBS_AudioMetadata& outMeta, MetadataPriority& outPriority) const
{
    QString domain, fileName;
    splitPath(filePath, domain, fileName);
    return query(domain, fileName, currentFileSize, outMeta, outPriority);
}

bool BPBS_MetaCache::tryUpgrade(const QString& domainId, const QString& fileName, qint64 fileSize,
                                MetadataPriority priority, const BPBS_AudioMetadata& meta)
{
    if (domainId.isEmpty() || fileName.isEmpty()) return false;
    QString cleanDomain = normalizeDirPath(domainId);
    std::unique_lock<std::shared_mutex> lock(m_mutex);
    std::shared_ptr<BPBS_DomainMetaBucket> bucket;
    auto dit = m_domains.find(cleanDomain);
    if (dit != m_domains.end()) {
        bucket = dit.value();
    } else {
        bucket = std::make_shared<BPBS_DomainMetaBucket>();
        bucket->domainId = cleanDomain;
        m_domains.insert(cleanDomain, bucket);
        if (m_activeDomain && m_activeDomain->domainId == cleanDomain) {
            m_activeDomain = bucket;
        }
    }
    auto fit = bucket->fileTable.find(fileName);
    if (fit != bucket->fileTable.end()) {
        BPBS_MetaRecord& record = fit.value();
        if (record.fileSize != fileSize) {
            record.fileSize = fileSize;
            record.priority = priority;
            record.metadata = meta;
            return true;
        }
        if (record.priority == MetadataPriority::InvalidAudio) {
            if (priority != MetadataPriority::WorkerDecoded && priority != MetadataPriority::ForegroundFocus) {
                return false;
            }
        }
        if (static_cast<int8_t>(priority) >= static_cast<int8_t>(record.priority)) {
            record.priority = priority;
            record.metadata = meta;
            return true;
        }
        return false;
    }
    BPBS_MetaRecord newRecord;
    newRecord.fileName = fileName;
    newRecord.fileSize = fileSize;
    newRecord.priority = priority;
    newRecord.metadata = meta;
    bucket->fileTable.insert(fileName, std::move(newRecord));
    return true;
}

bool BPBS_MetaCache::tryUpgrade(const QString& filePath, qint64 fileSize,
                                MetadataPriority priority, const BPBS_AudioMetadata& meta)
{
    QString domain, fileName;
    splitPath(filePath, domain, fileName);
    return tryUpgrade(domain, fileName, fileSize, priority, meta);
}

void BPBS_MetaCache::markInvalidAudio(const QString& domainId, const QString& fileName, qint64 fileSize) {
    if (domainId.isEmpty() || fileName.isEmpty()) return;
    QString cleanDomain = normalizeDirPath(domainId);
    std::unique_lock<std::shared_mutex> lock(m_mutex);
    std::shared_ptr<BPBS_DomainMetaBucket> bucket;
    auto dit = m_domains.find(cleanDomain);
    if (dit != m_domains.end()) {
        bucket = dit.value();
    } else {
        bucket = std::make_shared<BPBS_DomainMetaBucket>();
        bucket->domainId = cleanDomain;
        m_domains.insert(cleanDomain, bucket);
    }
    auto fit = bucket->fileTable.find(fileName);
    if (fit != bucket->fileTable.end()) {
        BPBS_MetaRecord& record = fit.value();
        record.fileSize = fileSize;
        record.priority = MetadataPriority::InvalidAudio;
        record.metadata = BPBS_AudioMetadata{};
    } else {
        BPBS_MetaRecord record;
        record.fileName = fileName;
        record.fileSize = fileSize;
        record.priority = MetadataPriority::InvalidAudio;
        bucket->fileTable.insert(fileName, std::move(record));
    }
}

void BPBS_MetaCache::markInvalidAudio(const QString& filePath, qint64 fileSize) {
    QString domain, fileName;
    splitPath(filePath, domain, fileName);
    markInvalidAudio(domain, fileName, fileSize);
}

void BPBS_MetaCache::clear() {
    std::unique_lock<std::shared_mutex> lock(m_mutex);
    m_domains.clear();
    m_activeDomain.reset();
}

void BPBS_MetaCache::clearDomain(const QString& domainId) {
    QString cleanDomain = normalizeDirPath(domainId);
    std::unique_lock<std::shared_mutex> lock(m_mutex);
    m_domains.remove(cleanDomain);
    if (m_activeDomain && m_activeDomain->domainId == cleanDomain) {
        m_activeDomain.reset();
    }
}

int BPBS_MetaCache::totalRecordsCount() const {
    std::shared_lock<std::shared_mutex> lock(m_mutex);
    int total = 0;
    for (auto it = m_domains.begin(); it != m_domains.end(); ++it) {
        total += it.value()->fileTable.size();
    }
    return total;
}

qint64 BPBS_MetaCache::estimatedMemoryUsage() const {
    std::shared_lock<std::shared_mutex> lock(m_mutex);
    qint64 total = sizeof(BPBS_MetaCache);
    for (auto dit = m_domains.begin(); dit != m_domains.end(); ++dit) {
        total += sizeof(BPBS_DomainMetaBucket);
        total += dit.key().capacity() * sizeof(QChar);
        const auto& table = dit.value()->fileTable;
        total += table.capacity() * (sizeof(void*) + sizeof(BPBS_MetaRecord));
        for (auto fit = table.begin(); fit != table.end(); ++fit) {
            const auto& r = fit.value();
            total += r.fileName.capacity() * sizeof(QChar);
            total += r.metadata.title.capacity() * sizeof(QChar);
            total += r.metadata.artist.capacity() * sizeof(QChar);
            total += r.metadata.album.capacity() * sizeof(QChar);
            total += r.metadata.codecName.capacity() * sizeof(QChar);
        }
    }
    return total;
}
#pragma once

#include "BPBS_Types.h"

#include <QString>
#include <QHash>
#include <memory>
#include <shared_mutex>
#include <cstdint>

enum class MetadataPriority : int8_t {
    InvalidAudio    = -1,
    Unprobed        =  0,
    FastProbe       =  1,
    WorkerDecoded   =  2,
    ForegroundFocus =  3
};

struct BPBS_MetaRecord {
    QString fileName;
    qint64 fileSize = 0;
    MetadataPriority priority = MetadataPriority::Unprobed;
    BPBS_AudioMetadata metadata;
};

struct BPBS_DomainMetaBucket {
    QString domainId;
    QHash<QString, BPBS_MetaRecord> fileTable;
};

class BPBS_MetaCache {
public:
    BPBS_MetaCache() = default;
    ~BPBS_MetaCache() = default;
    BPBS_MetaCache(const BPBS_MetaCache&) = delete;
    BPBS_MetaCache& operator=(const BPBS_MetaCache&) = delete;
    void setCurrentDomain(const QString& domainId);
    [[nodiscard]] MetadataPriority getPriority(const QString& domainId, const QString& fileName, qint64 currentFileSize) const;
    [[nodiscard]] MetadataPriority getPriority(const QString& filePath, qint64 currentFileSize) const;
    [[nodiscard]] bool isKnownInvalid(const QString& domainId, const QString& fileName, qint64 currentFileSize) const;
    [[nodiscard]] bool isKnownInvalid(const QString& filePath, qint64 currentFileSize) const;
    bool query(const QString& domainId, const QString& fileName, qint64 currentFileSize, BPBS_AudioMetadata& outMeta, MetadataPriority& outPriority) const;
    bool query(const QString& filePath, qint64 currentFileSize, BPBS_AudioMetadata& outMeta, MetadataPriority& outPriority) const;
    bool tryUpgrade(const QString& domainId, const QString& fileName, qint64 fileSize, MetadataPriority priority, const BPBS_AudioMetadata& meta);
    bool tryUpgrade(const QString& filePath, qint64 fileSize, MetadataPriority priority, const BPBS_AudioMetadata& meta);
    void markInvalidAudio(const QString& domainId, const QString& fileName, qint64 fileSize);
    void markInvalidAudio(const QString& filePath, qint64 fileSize);
    void clear();
    void clearDomain(const QString& domainId);
    [[nodiscard]] int totalRecordsCount() const;
    [[nodiscard]] qint64 estimatedMemoryUsage() const;

private:
    static void splitPath(const QString& fullPath, QString& outDomain, QString& outFileName);
    mutable std::shared_mutex m_mutex;
    QHash<QString, std::shared_ptr<BPBS_DomainMetaBucket>> m_domains;
    std::shared_ptr<BPBS_DomainMetaBucket> m_activeDomain;
};
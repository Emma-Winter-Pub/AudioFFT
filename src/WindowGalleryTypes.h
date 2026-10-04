#pragma once

#include "WindowNavBarViewMode.h"
#include "BPBS_MetaCache.h"

#include <QString>
#include <QDateTime>
#include <QIcon>
#include <QList>
#include <QMetaType>
#include <cstdint>
#include <limits>

enum class GalleryItemCategory {
    Folder,
    AudioFile,
    OtherFile
};
Q_DECLARE_METATYPE(GalleryItemCategory)

enum class GalleryColumnType {
    Name = 0,
    DateModified = 1,
    Type = 2,
    Size = 3,
    DateCreated = 4,
    FolderPath = 5,
    Duration = 6,
    Bitrate = 7,
    SampleRate = 8,
    BitDepth = 9,
    Channels = 10,
    Codec = 11,
    Title = 12,
    Artist = 13,
    Album = 14,
    Track = 15,
    ColumnCount = 16
};
Q_DECLARE_METATYPE(GalleryColumnType)

struct GalleryItemData {
    QString fileName;
    QString absolutePath;
    QString folderPath;
    QString fileExtension;
    GalleryItemCategory category = GalleryItemCategory::OtherFile;
    bool isDirectory = false;
    qint64 lastModifiedMsecs = std::numeric_limits<qint64>::min();
    bool isDateValid = false;
    QString formattedDate;
    qint64 createdMsecs = std::numeric_limits<qint64>::min();
    bool isCreatedDateValid = false;
    QString formattedCreatedDate;
    qint64 sizeBytes = 0;
    QString formattedSize;
    QString typeDescription;
    QIcon icon;
    bool isAudio = false;
    bool isMetadataProbed = false;
    int metadataPriority = 0;
    double durationSec = 0.0;
    QString formattedDuration;
    long long bitrate = 0;
    QString formattedBitrate;
    int sampleRate = 0;
    QString formattedSampleRate;
    int bitDepth = 0;
    int channels = 0;
    QString codecName;
    QString title;
    QString artist;
    QString album;
    QString track;
};
Q_DECLARE_METATYPE(GalleryItemData)
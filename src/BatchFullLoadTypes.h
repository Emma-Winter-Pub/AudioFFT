#pragma once

#include "MappingCurves.h"
#include "FFTWindowFunctions.h"
#include "BatchFullLoadConfig.h"
#include "FFmpegMemLoader.h"
#include "AudioExtensionList.h"

#include <QString>
#include <QList>
#include <vector>
#include <QMap>
#include <QHash>
#include <QByteArray>

extern "C" {
#include <libavcodec/avcodec.h>
}

enum class BatchMode {
    FullLoad,
    Streaming
};

struct FilePerformanceMetrics {
    QString filePath;
    qint64 decodingTimeMs = 0;
    qint64 fftTimeMs = 0;
    qint64 renderingTimeMs = 0;
};

struct FileInfo {
    QString path;
    qint64 duration;
    QString codecName;
    AVCodecID codecId;
    QString outputFilePath;
};

Q_DECLARE_TYPEINFO(FileInfo, Q_MOVABLE_TYPE);

using FileSnapshot = QHash<QString, qint64>;

struct BatchTask {
    FileInfo info;
    SharedFileBuffer memoryData;
    bool isLoadedInMemory = false;
};

struct BatchWriteTask {
    QString outputPath;
    QByteArray encodedData;
    int width = 0;
    int height = 0;
    QString sourceFilePath;
    QString successInfo;
    FilePerformanceMetrics metrics;
    int bucketId = 0;
};

Q_DECLARE_TYPEINFO(BatchWriteTask, Q_MOVABLE_TYPE);

struct BatchSettings {
    BatchMode mode = BatchMode::FullLoad;
    int threadCount = 0;
    QString inputPath;
    QString outputPath;
    bool includeSubfolders = BatchFullLoadConfig::DEFAULT_INCLUDE_SUBFOLDERS;
    bool reuseSubfolderStructure = BatchFullLoadConfig::DEFAULT_REUSE_STRUCTURE;
    bool enableGrid = BatchFullLoadConfig::DEFAULT_ENABLE_GRID;
    bool enableComponents = true;
    bool enableWidthLimit = BatchFullLoadConfig::DEFAULT_ENABLE_WIDTH_LIMIT;
    int maxWidth = BatchFullLoadConfig::DEFAULT_MAX_WIDTH;
    int imageHeight = BatchFullLoadConfig::DEFAULT_IMAGE_HEIGHT;
    double timeInterval = BatchFullLoadConfig::DEFAULT_TIME_INTERVAL;
    QString exportFormat = BatchFullLoadConfig::DEFAULT_EXPORT_FORMAT;
    int qualityLevel = BatchFullLoadConfig::DEFAULT_QUALITY_LEVEL;
    CurveType curveType = BatchFullLoadConfig::DEFAULT_CURVE_TYPE;
    FFTWindowType windowType = BatchFullLoadConfig::DEFAULT_WINDOW_TYPE;
    QString paletteId = "0000";
    bool paletteInverted = false;
    bool paletteNegative = false;
    double minDb = BatchFullLoadConfig::DEFAULT_MIN_DB;
    double maxDb = BatchFullLoadConfig::DEFAULT_MAX_DB;
    bool enableWhitelist = false;
    QStringList whitelistExtensions = AudioExtensionList::getDefaultExtensions();
    bool excludeVideoFiles = false;
    bool categorizeByCodec = false;
    bool operator==(const BatchSettings& other) const {
        return mode == other.mode &&
               threadCount == other.threadCount &&
               inputPath == other.inputPath &&
               outputPath == other.outputPath &&
               includeSubfolders == other.includeSubfolders &&
               reuseSubfolderStructure == other.reuseSubfolderStructure &&
               enableGrid == other.enableGrid &&
               enableComponents == other.enableComponents &&
               enableWidthLimit == other.enableWidthLimit &&
               maxWidth == other.maxWidth &&
               imageHeight == other.imageHeight &&
               std::abs(timeInterval - other.timeInterval) < 1e-9 &&
               exportFormat == other.exportFormat &&
               qualityLevel == other.qualityLevel &&
               curveType == other.curveType &&
               windowType == other.windowType &&
               paletteId == other.paletteId &&
               paletteInverted == other.paletteInverted &&
               paletteNegative == other.paletteNegative &&
               std::abs(minDb - other.minDb) < 1e-9 &&
               std::abs(maxDb - other.maxDb) < 1e-9 &&
               enableWhitelist == other.enableWhitelist &&
               whitelistExtensions == other.whitelistExtensions &&
               excludeVideoFiles == other.excludeVideoFiles &&
               categorizeByCodec == other.categorizeByCodec;
    }
    bool operator!=(const BatchSettings& other) const {
        return !(*this == other);
    }
};
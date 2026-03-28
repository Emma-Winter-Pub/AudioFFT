#pragma once

#include "MappingCurves.h"
#include "WindowFunctions.h"
#include "BatConfig.h"
#include "ColorPalette.h"
#include "FFmpegMemLoader.h"

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
};
Q_DECLARE_TYPEINFO(FileInfo, Q_MOVABLE_TYPE);

using FileSnapshot = QHash<QString, qint64>;

struct BatTask {
    FileInfo info;
    SharedFileBuffer memoryData;
    bool isLoadedInMemory = false;
};

struct BatWriteTask {
    QString outputPath;
    QByteArray encodedData;
    int width = 0;
    int height = 0;
};

Q_DECLARE_TYPEINFO(BatWriteTask, Q_MOVABLE_TYPE);

struct BatSettings {
    BatchMode mode = BatchMode::FullLoad;
    int threadCount = 0;
    QString inputPath;
    QString outputPath;
    bool includeSubfolders = BatConfig::DEFAULT_INCLUDE_SUBFOLDERS;
    bool reuseSubfolderStructure = BatConfig::DEFAULT_REUSE_STRUCTURE;
    bool enableGrid = BatConfig::DEFAULT_ENABLE_GRID;
    bool enableComponents = true;
    bool enableWidthLimit = BatConfig::DEFAULT_ENABLE_WIDTH_LIMIT;
    int maxWidth = BatConfig::DEFAULT_MAX_WIDTH;
    int imageHeight = BatConfig::DEFAULT_IMAGE_HEIGHT;
    double timeInterval = BatConfig::DEFAULT_TIME_INTERVAL;
    QString exportFormat = BatConfig::DEFAULT_EXPORT_FORMAT;
    int qualityLevel = BatConfig::DEFAULT_QUALITY_LEVEL;
    CurveType curveType = BatConfig::DEFAULT_CURVE_TYPE;
    WindowType windowType = BatConfig::DEFAULT_WINDOW_TYPE;
    PaletteType paletteType = BatConfig::DEFAULT_PALETTE_TYPE;
    double minDb = BatConfig::DEFAULT_MIN_DB;
    double maxDb = BatConfig::DEFAULT_MAX_DB;
    bool operator==(const BatSettings& other) const {
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
               std::abs(minDb - other.minDb) < 1e-9 &&
               std::abs(maxDb - other.maxDb) < 1e-9 &&
               paletteType == other.paletteType;
    }
    bool operator!=(const BatSettings& other) const {
        return !(*this == other);
    }
};
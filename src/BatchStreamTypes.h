#pragma once

#include "AlignedAllocator.h"
#include "FFTWindowFunctions.h"
#include "MappingCurves.h"
#include "ColorPaletteFactory.h"
#include "BatchFullLoadConfig.h"
#include "AudioExtensionList.h"

#include <vector>
#include <variant>
#include <QString>
#include <QByteArray>

using BatchStreamPcm32 = std::vector<float, AlignedAllocator<float, 64>>;
using BatchStreamPcm64 = std::vector<double, AlignedAllocator<double, 64>>;
using BatchStreamPcmVariant = std::variant<BatchStreamPcm32, BatchStreamPcm64>;

struct BatchStreamSettings {
    int threadCount = 0;
    QString inputPath;
    QString outputPath;
    bool useMultiThreading = true;
    bool includeSubfolders = BatchFullLoadConfig::DEFAULT_INCLUDE_SUBFOLDERS;
    bool reuseSubfolderStructure = BatchFullLoadConfig::DEFAULT_REUSE_STRUCTURE;
    int imageHeight = BatchFullLoadConfig::DEFAULT_IMAGE_HEIGHT;
    double timeInterval = BatchFullLoadConfig::DEFAULT_TIME_INTERVAL;
    FFTWindowType windowType = BatchFullLoadConfig::DEFAULT_WINDOW_TYPE;
    CurveType curveType = BatchFullLoadConfig::DEFAULT_CURVE_TYPE;
    QString paletteId = "0000";
    bool paletteInverted = false;
    bool paletteNegative = false;
    double minDb = BatchFullLoadConfig::DEFAULT_MIN_DB;
    double maxDb = BatchFullLoadConfig::DEFAULT_MAX_DB;
    bool enableGrid = BatchFullLoadConfig::DEFAULT_ENABLE_GRID;
    bool enableComponents = true;
    bool enableWidthLimit = BatchFullLoadConfig::DEFAULT_ENABLE_WIDTH_LIMIT;
    int maxWidth = BatchFullLoadConfig::DEFAULT_MAX_WIDTH;
    QString exportFormat = BatchFullLoadConfig::DEFAULT_EXPORT_FORMAT;
    int qualityLevel = BatchFullLoadConfig::DEFAULT_QUALITY_LEVEL;
    bool enableWhitelist = false;
    QStringList whitelistExtensions = AudioExtensionList::getDefaultExtensions();
    bool excludeVideoFiles = false;
    bool categorizeByCodec = false;
};

struct BatchStreamAudioInfo {
    QString filePath;
    int sampleRate = 0;
    int channels = 0;
    double durationSec = 0.0;
    int sourceBitDepth = 16;
    long long totalSamplesEst = 0;
};

struct BatchStreamWriteTask {
    QString outputPath;
    QByteArray encodedData;
    int width = 0;
    int height = 0;
    QString sourceFilePath;
    QString successInfo;
    qint64 elapsedMs = 0;
    int bucketId = 0;
};

Q_DECLARE_TYPEINFO(BatchStreamWriteTask, Q_MOVABLE_TYPE);
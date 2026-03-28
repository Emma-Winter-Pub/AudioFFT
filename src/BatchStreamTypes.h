#pragma once

#include "AlignedAllocator.h"
#include "WindowFunctions.h"
#include "MappingCurves.h"
#include "ColorPalette.h"
#include "BatConfig.h"

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
    bool includeSubfolders = BatConfig::DEFAULT_INCLUDE_SUBFOLDERS;
    bool reuseSubfolderStructure = BatConfig::DEFAULT_REUSE_STRUCTURE;
    int imageHeight = BatConfig::DEFAULT_IMAGE_HEIGHT;
    double timeInterval = BatConfig::DEFAULT_TIME_INTERVAL;
    WindowType windowType = BatConfig::DEFAULT_WINDOW_TYPE;
    CurveType curveType = BatConfig::DEFAULT_CURVE_TYPE;
    PaletteType paletteType = BatConfig::DEFAULT_PALETTE_TYPE;
    double minDb = BatConfig::DEFAULT_MIN_DB;
    double maxDb = BatConfig::DEFAULT_MAX_DB;
    bool enableGrid = BatConfig::DEFAULT_ENABLE_GRID;
    bool enableComponents = true;
    bool enableWidthLimit = BatConfig::DEFAULT_ENABLE_WIDTH_LIMIT;
    int maxWidth = BatConfig::DEFAULT_MAX_WIDTH;
    QString exportFormat = BatConfig::DEFAULT_EXPORT_FORMAT;
    int qualityLevel = BatConfig::DEFAULT_QUALITY_LEVEL;
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
};
Q_DECLARE_TYPEINFO(BatchStreamWriteTask, Q_MOVABLE_TYPE);
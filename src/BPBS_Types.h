#pragma once

#include "AlignedAllocator.h"
#include "FFTWindowFunctions.h"
#include "MappingCurves.h"
#include "ColorPaletteFactory.h"
#include "BatchFullLoadConfig.h"
#include "AudioExtensionList.h"
#include "BPBS_Buffer.h"

#include <vector>
#include <variant>
#include <memory>
#include <cmath>
#include <cstdint>
#include <QString>
#include <QByteArray>
#include <QList>
#include <QStringList>

class BPBS_CancellationToken;

using BPBS_Pcm32 = std::vector<float, AlignedAllocator<float, 64>>;
using BPBS_Pcm64 = std::vector<double, AlignedAllocator<double, 64>>;
using BPBS_PcmVariant = std::variant<BPBS_Pcm32, BPBS_Pcm64>;

enum class BPBS_Precision {
    Float32,
    Float64
};

struct BPBS_CalcParams {
    int imageHeight = BatchFullLoadConfig::DEFAULT_IMAGE_HEIGHT;
    double timeInterval = BatchFullLoadConfig::DEFAULT_TIME_INTERVAL;
    FFTWindowType windowType = BatchFullLoadConfig::DEFAULT_WINDOW_TYPE;
    CurveType curveType = BatchFullLoadConfig::DEFAULT_CURVE_TYPE;
    double minDb = BatchFullLoadConfig::DEFAULT_MIN_DB;
    double maxDb = BatchFullLoadConfig::DEFAULT_MAX_DB;
    [[nodiscard]] uint64_t toFingerprint() const noexcept {
        uint64_t h = 14695981039346656037ULL;
        auto hashCombine = [&h](const void* data, size_t len) noexcept {
            const auto* p = static_cast<const uint8_t*>(data);
            for (size_t i = 0; i < len; ++i) {
                h ^= static_cast<uint64_t>(p[i]);
                h *= 1099511628211ULL;
            }
        };
        hashCombine(&imageHeight, sizeof(imageHeight));
        double rInterval = (timeInterval <= 1e-9) ? 0.0 : timeInterval;
        hashCombine(&rInterval, sizeof(rInterval));
        int winInt = static_cast<int>(windowType);
        hashCombine(&winInt, sizeof(winInt));
        int curveInt = static_cast<int>(curveType);
        hashCombine(&curveInt, sizeof(curveInt));
        double rMinDb = std::round(minDb * 100.0) / 100.0;
        double rMaxDb = std::round(maxDb * 100.0) / 100.0;
        hashCombine(&rMinDb, sizeof(rMinDb));
        hashCombine(&rMaxDb, sizeof(rMaxDb));
        return h;
    }
    bool operator==(const BPBS_CalcParams& o) const noexcept {
        return imageHeight == o.imageHeight &&
               std::abs(timeInterval - o.timeInterval) < 1e-9 &&
               windowType == o.windowType &&
               curveType == o.curveType &&
               std::abs(minDb - o.minDb) < 1e-9 &&
               std::abs(maxDb - o.maxDb) < 1e-9;
    }
    bool operator!=(const BPBS_CalcParams& o) const noexcept {
        return !(*this == o);
    }
};

struct BPBS_VisualParams {
    QString paletteId = "0000";
    bool paletteInverted = false;
    bool paletteNegative = false;
    bool operator==(const BPBS_VisualParams& o) const noexcept {
        return paletteId == o.paletteId &&
               paletteInverted == o.paletteInverted &&
               paletteNegative == o.paletteNegative;
    }
    bool operator!=(const BPBS_VisualParams& o) const noexcept {
        return !(*this == o);
    }
};

enum class BPBS_PreemptResult {
    NotOwned,
    Preempted_SSD,
    Preempted_HDD
};

struct BPBS_DemandItem {
    QString filePath;
    int rank = 0;
    qint64 fileSize = 0;
    int codecId = 0;
    qint64 durationMs = 0;
    int distance = 0;
    std::shared_ptr<BPBS_Buffer> buffer;
};

struct BPBS_WorkerSlot {
    int workerId = -1;
    QString filePath;
    std::shared_ptr<BPBS_CancellationToken> token;
    std::shared_ptr<BPBS_Buffer> buffer;
    [[nodiscard]] bool isBusy() const noexcept { return !filePath.isEmpty(); }
    void clear() noexcept {
        filePath.clear();
        token.reset();
        if (buffer) {
            buffer->abort();
            buffer.reset();
        }
    }
};

struct BPBS_DomainContext {
    QString domainId;
    qint64 leaveTime = 0;
    bool isCurrent = false;
};

struct BPBS_TrackInfo {
    int index = 0;
    int channels = 0;
    QString codecName;
    QString channelLayout;
    QStringList channelNames;
};

struct BPBS_AudioMetadata {
    QString filePath;
    qint64 fileSizeBytes = 0;
    QString formatName;
    QString formatLongName;
    QString codecName;
    QString codecLongName;
    int sampleRateHz = 0;
    qint64 bitrateBps = 0;
    int sourceBitDepth = 0;
    double durationSeconds = 0.0;
    qint64 durationMicroseconds = 0;
    QList<BPBS_TrackInfo> tracks;
    int defaultTrackIndex = -1;
    QString title;
    QString artist;
    QString album;
    QString track;
    QString date;
};

struct BPBS_Settings {
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
    double minDb = BatchFullLoadConfig::DEFAULT_MIN_DB;
    double maxDb = BatchFullLoadConfig::DEFAULT_MAX_DB;
    QString paletteId = "0000";
    bool paletteInverted = false;
    bool paletteNegative = false;
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
    [[nodiscard]] BPBS_CalcParams calcParams() const noexcept {
        BPBS_CalcParams cp;
        cp.imageHeight = imageHeight;
        cp.timeInterval = timeInterval;
        cp.windowType = windowType;
        cp.curveType = curveType;
        cp.minDb = minDb;
        cp.maxDb = maxDb;
        return cp;
    }
    [[nodiscard]] BPBS_VisualParams visualParams() const noexcept {
        BPBS_VisualParams vp;
        vp.paletteId = paletteId;
        vp.paletteInverted = paletteInverted;
        vp.paletteNegative = paletteNegative;
        return vp;
    }
};
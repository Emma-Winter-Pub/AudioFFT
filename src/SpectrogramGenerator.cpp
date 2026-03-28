#include "SpectrogramGenerator.h"
#include "AppConfig.h"
#include "Utils.h"
#include "MappingCurves.h"
#include "ColorPalette.h"

#include <vector>
#include <cmath>
#include <algorithm>
#include <QtConcurrent>
#include <QPainter>
#include <stdexcept>
#include <cstring> 
#include <QTime>
#include <variant>
#include <cstdint>

namespace {
    template <size_t N, typename T>
    inline const T* assume_aligned_ptr(const T* ptr) {
#if defined(_MSC_VER)
        __assume(reinterpret_cast<uintptr_t>(ptr) % N == 0);
        return ptr;
#elif defined(__GNUC__) || defined(__clang__)
        return static_cast<const T*>(__builtin_assume_aligned(ptr, N));
#else
        return ptr;
#endif
    }
}

SpectrogramGenerator::SpectrogramGenerator(QObject* parent) : QObject(parent) {}

struct RenderChunk { int start_x; int width; };

QImage SpectrogramGenerator::generate(
    const SpectrumDataVariant& spectrogramData,
    int fftSize,
    int targetHeight,
    int sampleRate,
    CurveType curveType,
    double minDb,
    double maxDb,
    PaletteType paletteType)
{
    if (fftSize <= 0 || targetHeight <= 0 || sampleRate <= 0) {
        emit logMessage(tr("    错误：无法生成图像 参数无效"));
        return QImage();
    }
    const double maxFreq = static_cast<double>(sampleRate) / 2.0;
    const int numBins = fftSize / 2 + 1;
    std::vector<QColor> qColors = ColorPalette::getPalette(paletteType);
    if (qColors.size() < 256) qColors = ColorPalette::getPalette(PaletteType::S00);
    QList<QRgb> colorTable;
    colorTable.reserve(256);
    for (const auto& color : qColors) colorTable.append(color.rgb());
    std::vector<int> y_to_bin_map(targetHeight);
    const double max_bin_index = numBins > 1 ? numBins - 1 : 0;
    const double inv_height_minus_1 = targetHeight > 1 ? 1.0 / (targetHeight - 1) : 0.0;
    for (int y = 0; y < targetHeight; ++y) {
        double y_ratio = y * inv_height_minus_1;
        double linear_freq_ratio = MappingCurves::inverse(y_ratio, curveType, maxFreq);
        if (linear_freq_ratio < 0.0) linear_freq_ratio = 0.0;
        if (linear_freq_ratio > 1.0) linear_freq_ratio = 1.0;
        y_to_bin_map[y] = static_cast<int>(std::round(linear_freq_ratio * max_bin_index));
    }
    auto computeWorker = [&](auto&& flatData) -> QImage {
        using T = typename std::decay_t<decltype(flatData)>::value_type;
        if (flatData.empty()) return QImage();
        const int imageWidth = static_cast<int>(flatData.size() / numBins);
        if (imageWidth <= 0) return QImage();
        const int numThreads = QThread::idealThreadCount();
        const int numChunks = (imageWidth < numThreads * 4) ? 1 : numThreads;
        QList<RenderChunk> chunks;
        chunks.reserve(numChunks);
        for (int i = 0; i < numChunks; ++i) {
            int start = (static_cast<long long>(i) * imageWidth) / numChunks;
            int end = (static_cast<long long>(i + 1) * imageWidth) / numChunks;
            if (end > start) chunks.append({ start, end - start });
        }
        const int* yToBinPtr = y_to_bin_map.data();
        const T* dataPtr = assume_aligned_ptr<64>(flatData.data());
        const double db_range = maxDb - minDb;
        const double inv_db_range = (db_range > 1e-9) ? 1.0 / db_range : 0.0;
        auto renderTask = [=](const RenderChunk& chunk) -> QImage {
            QImage chunkImage(chunk.width, targetHeight, QImage::Format_Indexed8);
            for (int y_img = 0; y_img < targetHeight; ++y_img) {
                int spectrogram_y = targetHeight - 1 - y_img;
                int bin_index = yToBinPtr[spectrogram_y];
                uchar* scanLine = chunkImage.scanLine(y_img);
                for (int x_local = 0; x_local < chunk.width; ++x_local) {
                    int x_global = chunk.start_x + x_local;
                    size_t dataIndex = static_cast<size_t>(x_global) * numBins + bin_index;
                    T db_value = dataPtr[dataIndex];
                    double normalized_value = (static_cast<double>(db_value) - minDb) * inv_db_range;
                    uchar color_index;
                    if (normalized_value <= 0.0) color_index = 0;
                    else if (normalized_value >= 1.0) color_index = 255;
                    else color_index = static_cast<uchar>(normalized_value * 255.0);
                    scanLine[x_local] = color_index;
                }
            }
            return chunkImage;
            };
        QImage resultImage(imageWidth, targetHeight, QImage::Format_Indexed8);
        resultImage.setColorTable(colorTable);
        try {
            QList<QImage> renderedChunks = QtConcurrent::blockingMapped(chunks, renderTask);
            for (int i = 0; i < chunks.size(); ++i) {
                const QImage& chunkImg = renderedChunks[i];
                int startX = chunks[i].start_x;
                int validWidthBytes = chunks[i].width;
                for (int y = 0; y < targetHeight; ++y) {
                    uchar* dest = resultImage.scanLine(y) + startX;
                    const uchar* src = chunkImg.constScanLine(y);
                    std::memcpy(dest, src, validWidthBytes);
                }
            }
        }
        catch (...) {
            return QImage();
        }
        emit logMessage(tr("        [分辨率] %1×%2")
            .arg(imageWidth)
            .arg(targetHeight));
        return resultImage;
        };
    return std::visit(computeWorker, spectrogramData);
}
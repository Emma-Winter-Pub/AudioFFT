#include "BatchFullLoadSpectrogramGenerator.h"
#include "BatchFullLoadConfig.h"
#include "MappingCurves.h"
#include "ColorPaletteFactory.h"

#include <vector>
#include <cmath>
#include <algorithm>
#include <stdexcept>
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

BatchFullLoadSpectrogramGenerator::BatchFullLoadSpectrogramGenerator() {}

BatchFullLoadSpectrogramGenerator::~BatchFullLoadSpectrogramGenerator() {}

QImage BatchFullLoadSpectrogramGenerator::generate(
    const SpectrumDataVariant& spectrogramData,
    int fftSize,
    int targetHeight,
    int sampleRate,
    CurveType curveType,
    double minDb,
    double maxDb, 
    const QString& paletteId,
    bool paletteInverted,
    bool paletteNegative)
{
    if (fftSize <= 0 || targetHeight <= 0 || sampleRate <= 0) {
        return QImage();
    }
    const double maxFreq = static_cast<double>(sampleRate) / 2.0;
    const int numBins = fftSize / 2 + 1;
    QList<QRgb> colorTable = ColorPaletteFactory::instance().getPalette(paletteId, paletteInverted, paletteNegative);
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
        QImage resultImage(imageWidth, targetHeight, QImage::Format_Indexed8);
        if (resultImage.isNull()) {
            return QImage();
        }
        resultImage.setColorTable(colorTable);
        const double db_range = maxDb - minDb;
        const double inv_db_range = (db_range > 1e-9) ? 1.0 / db_range : 0.0;
        const T* dataPtr = assume_aligned_ptr<64>(flatData.data());
        const int* yToBinPtr = y_to_bin_map.data();
        std::vector<uchar*> scanLines(targetHeight);
        for (int y = 0; y < targetHeight; ++y) {
            scanLines[y] = resultImage.scanLine(y);
        }
        for (int x = 0; x < imageWidth; ++x) {
            size_t dataBaseIndex = static_cast<size_t>(x) * numBins;
            for (int y_img = 0; y_img < targetHeight; ++y_img) {
                int spectrogram_y = targetHeight - 1 - y_img;
                int bin_index = yToBinPtr[spectrogram_y];
                T db_value = dataPtr[dataBaseIndex + bin_index];
                double normalized_value = (static_cast<double>(db_value) - minDb) * inv_db_range;
                uchar color_index;
                if (normalized_value <= 0.0) color_index = 0;
                else if (normalized_value >= 1.0) color_index = 255;
                else color_index = static_cast<uchar>(normalized_value * 255.0);
                scanLines[y_img][x] = color_index;
            }
        }
        return resultImage;
    };
    return std::visit(computeWorker, spectrogramData);
}
#include "BatSpectrogramGenerator.h"
#include "BatConfig.h"
#include "MappingCurves.h"

#include <cmath>
#include <vector>
#include <algorithm>
#include <stdexcept>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif


namespace { 

const std::vector<QRgb>& getPrecomputedColorLUT() {
    static const std::vector<QRgb> lut = []() -> std::vector<QRgb> {
        auto colorMap = BatConfig::getColorMap();
        if (colorMap.size() < 256) {
            std::vector<QRgb> fallback_lut(256);
            for (int i = 0; i < 256; ++i) {
                fallback_lut[i] = qRgb(i, i, i);}
            return fallback_lut;}

        std::vector<QRgb> computed_lut(256);
        for (int i = 0; i < 256; ++i) {
            const QColor& c = colorMap[i];
            computed_lut[i] = qRgb(c.red(), c.green(), c.blue());}

        return computed_lut;}();

    return lut;
}

} // namespace


BatSpectrogramGenerator::BatSpectrogramGenerator(){}


BatSpectrogramGenerator::~BatSpectrogramGenerator(){}


QImage BatSpectrogramGenerator::generate(
    const std::vector<std::vector<double>>& spectrogramData,
    int targetHeight,
    CurveType curveType)
{
    if (spectrogramData.empty() || spectrogramData[0].empty() || targetHeight <= 0) {
        return QImage();}

    const int imageWidth = static_cast<int>(spectrogramData.size());
    const int numBins = static_cast<int>(spectrogramData[0].size());

    QImage resultImage(imageWidth, targetHeight, QImage::Format_RGB32);
    if (resultImage.isNull()) {
        return QImage();}

    resultImage.fill(Qt::black);

    const auto& colorLUT = getPrecomputedColorLUT();
    const double db_range = BatConfig::MAX_DB - BatConfig::MIN_DB;
    const double inv_db_range = (db_range > 1e-9) ? 1.0 / db_range : 0.0;

    std::vector<int> y_to_bin_map(targetHeight);
    const double max_bin_index = numBins > 1 ? numBins - 1 : 0;
    const double inv_height_minus_1 = targetHeight > 1 ? 1.0 / (targetHeight - 1) : 0.0;

    auto get_inverse_mapped_ratio = [&](double y_ratio) -> double {

        switch (curveType) {
            case CurveType::Function0:
                return y_ratio;
            
            case CurveType::Function1:
                return expm1(y_ratio * log(100.0)) / 99.0;
            
            case CurveType::Function2:
                return (100.0 - pow(100.0, 1.0 - y_ratio)) / 99.0;

            case CurveType::Function3:
                return 2.0 * asin(y_ratio) / M_PI;

            case CurveType::Function4:
                return 2.0 * acos(1.0 - y_ratio) / M_PI;

            case CurveType::Function5:
                return sqrt(y_ratio);

            case CurveType::Function6:
                return 1.0 - sqrt(1.0 - y_ratio);

            case CurveType::Function7:
                return cbrt(y_ratio);

            case CurveType::Function8:
                return cbrt(y_ratio - 1.0) + 1.0;

            case CurveType::Function9:{
                const double ARCTAN_FACTOR = 2.0 * tan(9.0 * M_PI / 20.0);
                const double ARCTAN_DIVISOR = 10.0 / (9.0 * M_PI);
                return 0.5 + atan(ARCTAN_FACTOR * (y_ratio - 0.5)) * ARCTAN_DIVISOR;}

            case CurveType::Function10:{
                const double TAN_DIVISOR = 1.0 / (2.0 * tan(9.0 * M_PI / 20.0));
                return 0.5 + tan(0.9 * M_PI * (y_ratio - 0.5)) * TAN_DIVISOR;}

            case CurveType::Function11:{
                double val = 2.0 * y_ratio - 1.0;
                if (val < 0) {
                    return 0.5 * (1.0 - pow(-val, 1.0 / 3.0));}
                    else {
                        return 0.5 * (1.0 + pow(val, 1.0 / 3.0));}}

            case CurveType::Function12:{
                double y = y_ratio;
                if (y < 0.5) {
                    return cbrt(y / 4.0);}
                    else {
                        return cbrt((y - 1.0) / 4.0) + 1.0;}}

            default:
                return y_ratio;
        }
    };

    for (int y = 0; y < targetHeight; ++y) {
        double y_ratio = y * inv_height_minus_1;
        double linear_freq_ratio = get_inverse_mapped_ratio(y_ratio);
        y_to_bin_map[y] = static_cast<int>(std::round(linear_freq_ratio * max_bin_index));}
    
    for (int x = 0; x < imageWidth; ++x) {
        const std::vector<double>& column_data = spectrogramData[x];
        
        for (int y = 0; y < targetHeight; ++y) {
            const int bin_index = y_to_bin_map[y];
            const double db_value = column_data[bin_index];
            
            const double normalized_value = (db_value - BatConfig::MIN_DB) * inv_db_range;
            int color_index = static_cast<int>(normalized_value * 255.0);
            color_index = std::max(0, std::min(255, color_index));
            
            resultImage.setPixelColor(x, targetHeight - 1 - y, colorLUT[color_index]);}}

    return resultImage;
}
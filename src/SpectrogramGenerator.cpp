#include "SpectrogramGenerator.h"
#include "MappingCurves.h"
#include "AppConfig.h"
#include "Utils.h"

#include <cmath>
#include <string>
#include <vector>
#include <algorithm>
#include <stdexcept>
#include <QTime>
#include <QDebug>
#include <QPainter>
#include <QtConcurrent>


static const std::vector<QRgb>& getPrecomputedColorLUT() {
    static const std::vector<QRgb> lut = []() -> std::vector<QRgb> {
        try {
            auto colorMap = AppConfig::getColorMap();
            if (colorMap.size() < 256) {
                qWarning() << SpectrogramGenerator::tr("Spectrogram Generator: Invalid color map. Switching to grayscale.");
                std::vector<QRgb> fallback_lut(256);
                for (int i = 0; i < 256; ++i) fallback_lut[i] = qRgb(i, i, i);
                return fallback_lut;}
            std::vector<QRgb> computed_lut(256);
            for (int i = 0; i < 256; ++i) {
                QColor c = colorMap[i];
                computed_lut[i] = qRgb(c.red(), c.green(), c.blue());}
            return computed_lut;}
        catch (const std::exception& e) {
            qWarning() << SpectrogramGenerator::tr("Spectrogram Generator: Exception caught while initializing color map: ") << e.what() << SpectrogramGenerator::tr(" Switching to grayscale.");
            std::vector<QRgb> fallback_lut(256);
            for (int i = 0; i < 256; ++i) fallback_lut[i] = qRgb(i, i, i);
            return fallback_lut;}}();
    return lut;
}


SpectrogramGenerator::SpectrogramGenerator(QObject *parent) : QObject(parent) {}


struct RenderChunk {
    int start_x;
    int width;
};


QImage SpectrogramGenerator::generate(const std::vector<std::vector<double>>& spectrogramData, int targetHeight, CurveType curveType){

    const auto startTime = QTime::currentTime();
    emit logMessage(getCurrentTimestamp() + tr(" Image generation started"));

    if (spectrogramData.empty() || spectrogramData[0].empty() || targetHeight <= 0) {
        emit logMessage(tr("Cannot generate image: Input data is empty or target height is invalid."));
        return QImage();}

    const size_t reference_col_size = spectrogramData[0].size();
    for(const auto& col : spectrogramData) {
        if (col.size() != reference_col_size) {
            emit logMessage(tr("Cannot generate image: Spectrogram data column sizes are inconsistent."));
            return QImage();}}

    const int imageWidth = spectrogramData.size();
    QImage resultImage(imageWidth, targetHeight, QImage::Format_RGB32);
    if (resultImage.isNull()) {
        emit logMessage(tr("Cannot generate image: Failed to allocate memory for the final image."));
        return QImage();}

    resultImage.fill(Qt::black);

    QList<RenderChunk> chunks;

    try {
        const auto& colorLUT = getPrecomputedColorLUT();
        const double db_range = AppConfig::MAX_DB - AppConfig::MIN_DB;
        const double inv_db_range = (db_range > 1e-9) ? 1.0 / db_range : 0.0;
        
        const int num_bins_total = reference_col_size;
        
        std::vector<int> y_to_bin_map(targetHeight);
        const double max_bin_index = num_bins_total > 1 ? num_bins_total - 1 : 0;
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
                return y_ratio;}
    };

        for (int y = 0; y < targetHeight; ++y) {
            double y_ratio = y * inv_height_minus_1;
            double linear_freq_ratio = get_inverse_mapped_ratio(y_ratio);
            y_to_bin_map[y] = static_cast<int>(std::round(linear_freq_ratio * max_bin_index));}

        const int numThreads = QThread::idealThreadCount();
        const int numChunks = std::min(imageWidth, numThreads * 4);
        chunks.reserve(numChunks);
        for(int i = 0; i < numChunks; ++i) {
            int start = (static_cast<long long>(i) * imageWidth) / numChunks;
            int end = (static_cast<long long>(i + 1) * imageWidth) / numChunks;
            if (end > start) {
                chunks.append({start, end - start});}}
        
        auto renderChunk = [&](const RenderChunk& chunk) -> QImage {
            QImage chunkImage(chunk.width, targetHeight, QImage::Format_RGB32);
            if (chunkImage.isNull()) {
                throw std::runtime_error(tr("Failed to allocate memory for image chunk.").toStdString());}
            
            for (int x_local = 0; x_local < chunk.width; ++x_local) {
                const int x_global = chunk.start_x + x_local;
                const std::vector<double>& column_data = spectrogramData[x_global];
                
                for (int y_img = 0; y_img < targetHeight; ++y_img) {
                    const int bin_index = y_to_bin_map[y_img];
                    const double db_value = column_data[bin_index];
                    
                    int color_index = 0;
                    if (std::isfinite(db_value)) {
                        const double norm_linear = (db_value - AppConfig::MIN_DB) * inv_db_range;
                        color_index = static_cast<int>(norm_linear * 255.0);
                        color_index = std::max(0, std::min(255, color_index));}
                    
                    QRgb* line = reinterpret_cast<QRgb*>(chunkImage.scanLine(targetHeight - 1 - y_img));
                    line[x_local] = colorLUT[color_index];}}
            return chunkImage;};
        
        QList<QImage> renderedChunks = QtConcurrent::blockingMapped(chunks, renderChunk);

        QPainter painter(&resultImage);
        painter.setCompositionMode(QPainter::CompositionMode_Source);
        for(int i = 0; i < chunks.size(); ++i) {
            painter.drawImage(chunks[i].start_x, 0, renderedChunks[i]);}}

    catch (const std::exception& e) {
        emit logMessage(QString(tr("Error occurred during image generation: %1")).arg(e.what()));
        return QImage();}

    catch (...) {
        emit logMessage(tr("Severe error of unknown type occurred during image generation."));
        return QImage();}
    
    emit logMessage(QString(tr("%1 Image generation completed. Resolution: %2×%3"))
        .arg(getCurrentTimestamp())
        .arg(imageWidth)
        .arg(targetHeight)
        .arg(formatElapsedMsAsStopwatch(startTime.msecsTo(QTime::currentTime()))));

    return resultImage;
}
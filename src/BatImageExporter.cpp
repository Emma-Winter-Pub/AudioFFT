#include "BatImageExporter.h"
#include "BatWidget.h"
#include "BatAudioDecoder.h"
#include "BatConfig.h"
#include "BatUtils.h"
#include "AppConfig.h"
#include "ImageEncoderFactory.h"
#include "IImageEncoder.h"
#include "MappingCurves.h"


#include <QPainter>
#include <QFileInfo>
#include <QDateTime>
#include <QFontMetrics>
#include <QLinearGradient>
#include <cmath>
#include <memory>
#include <vector>
#include <algorithm>


BatImageExporter::BatImageExporter() {}


BatImageExporter::~BatImageExporter() {}


bool BatImageExporter::exportImage(

    const QImage& rawSpectrogram,
    const QString& outputFilePath,
    const QString& sourceFileName,
    const BatSettings& settings,
    const BatDecodedAudio& audioInfo)
{
    QImage imageToProcess = rawSpectrogram;

    if (settings.exportFormat == "JPG") {
        int estimatedFinalWidth = imageToProcess.width() + BatConfig::FINAL_IMAGE_LEFT_MARGIN + BatConfig::FINAL_IMAGE_RIGHT_MARGIN;
        if (estimatedFinalWidth > BatConfig::JPEG_MAX_DIMENSION) {
            int newRawWidth = BatConfig::JPEG_MAX_DIMENSION - BatConfig::FINAL_IMAGE_LEFT_MARGIN - BatConfig::FINAL_IMAGE_RIGHT_MARGIN;
            if (newRawWidth > 0) {
                imageToProcess = imageToProcess.scaled(newRawWidth, imageToProcess.height(), Qt::IgnoreAspectRatio, Qt::SmoothTransformation);}}}

    if (settings.exportFormat == "WebP") {
        int estimatedFinalWidth = imageToProcess.width() + BatConfig::FINAL_IMAGE_LEFT_MARGIN + BatConfig::FINAL_IMAGE_RIGHT_MARGIN;
        if (estimatedFinalWidth > AppConfig::MY_WEBP_MAX_DIMENSION) {
            int newRawWidth = AppConfig::MY_WEBP_MAX_DIMENSION - BatConfig::FINAL_IMAGE_LEFT_MARGIN - BatConfig::FINAL_IMAGE_RIGHT_MARGIN;
            if (newRawWidth > 0) {
                imageToProcess = imageToProcess.scaled(newRawWidth, imageToProcess.height(), Qt::IgnoreAspectRatio, Qt::SmoothTransformation);}}}

    if (settings.exportFormat == "AVIF") {
        int estimatedFinalWidth = imageToProcess.width() + BatConfig::FINAL_IMAGE_LEFT_MARGIN + BatConfig::FINAL_IMAGE_RIGHT_MARGIN;
        if (estimatedFinalWidth > AppConfig::MY_AVIF_MAX_DIMENSION) {
            int newRawWidth = AppConfig::MY_AVIF_MAX_DIMENSION - BatConfig::FINAL_IMAGE_LEFT_MARGIN - BatConfig::FINAL_IMAGE_RIGHT_MARGIN;
            if (newRawWidth > 0) {
                imageToProcess = imageToProcess.scaled(newRawWidth, imageToProcess.height(), Qt::IgnoreAspectRatio, Qt::SmoothTransformation);}}}

    if (settings.enableWidthLimit && imageToProcess.width() > settings.maxWidth) {
        imageToProcess = imageToProcess.scaled(settings.maxWidth, imageToProcess.height(), Qt::IgnoreAspectRatio, Qt::SmoothTransformation);}

    QFont titleFont("", 10);
    QFontMetrics fm(titleFont);
    int fileNameWidth = fm.horizontalAdvance(sourceFileName);
    int spectrogramWidth = imageToProcess.width();
    int contentWidth = std::max(fileNameWidth, spectrogramWidth);

    const int colorBarWidth = 15;
    const int finalWidth = contentWidth + BatConfig::FINAL_IMAGE_LEFT_MARGIN + BatConfig::FINAL_IMAGE_RIGHT_MARGIN;
    const int finalHeight = imageToProcess.height() + BatConfig::FINAL_IMAGE_TOP_MARGIN + BatConfig::FINAL_IMAGE_BOTTOM_MARGIN;

    QImage finalImage(finalWidth, finalHeight, QImage::Format_RGB32);
    finalImage.fill(QColor(59, 68, 83));

    QPainter painter(&finalImage);
    painter.setRenderHint(QPainter::Antialiasing);

    int spectrogramOffsetX = BatConfig::FINAL_IMAGE_LEFT_MARGIN + (contentWidth - spectrogramWidth) / 2;
    painter.drawImage(spectrogramOffsetX, BatConfig::FINAL_IMAGE_TOP_MARGIN, imageToProcess);

    painter.setPen(Qt::white);
    painter.setFont(titleFont);

    QRect titleRect(BatConfig::FINAL_IMAGE_LEFT_MARGIN, 0, contentWidth, BatConfig::FINAL_IMAGE_TOP_MARGIN);
    painter.drawText(titleRect, Qt::AlignVCenter | Qt::AlignLeft, sourceFileName);

    const double maxFreqKhz = (audioInfo.sampleRate / 2.0) / 1000.0;
    const int freqStep = calculateBestFreqStep(maxFreqKhz, imageToProcess.height());
    painter.drawText(QRect(0, BatConfig::FINAL_IMAGE_TOP_MARGIN - 25, BatConfig::FINAL_IMAGE_LEFT_MARGIN - 5, 20), Qt::AlignRight | Qt::AlignVCenter, "kHz");

    QFontMetrics fm_khz(painter.font());
    const int minLabelSpacing = fm_khz.height();
    int lastDrawnLabelY = -1000;

    for (int khz = 0; khz <= static_cast<int>(maxFreqKhz); khz += freqStep) {
        if (khz > maxFreqKhz) break;

        double linear_ratio = (maxFreqKhz > 0) ? (static_cast<double>(khz) / maxFreqKhz) : 0.0;
        double curved_ratio = MappingCurves::map(linear_ratio, settings.curveType);
        int y_pos = BatConfig::FINAL_IMAGE_TOP_MARGIN + static_cast<int>((1.0 - curved_ratio) * imageToProcess.height());

        if (std::abs(y_pos - lastDrawnLabelY) < minLabelSpacing && khz > 0) {
            continue;}

        painter.drawText(QRect(0, y_pos - 10, BatConfig::FINAL_IMAGE_LEFT_MARGIN - 5, 20), Qt::AlignRight | Qt::AlignVCenter, QString::number(khz));
        lastDrawnLabelY = y_pos;
    }

    QRect colorBarArea(finalWidth - BatConfig::FINAL_IMAGE_RIGHT_MARGIN + 5, BatConfig::FINAL_IMAGE_TOP_MARGIN, colorBarWidth, imageToProcess.height());
    QLinearGradient gradient(colorBarArea.topLeft(), colorBarArea.bottomLeft());
    auto colorMap = BatConfig::getColorMap();
    for (size_t i = 0; i < colorMap.size(); ++i) {
        gradient.setColorAt(1.0 - (static_cast<double>(i) / (colorMap.size() - 1)), colorMap[i]);}

    painter.fillRect(colorBarArea, gradient);
    painter.drawText(QRect(finalWidth - BatConfig::FINAL_IMAGE_RIGHT_MARGIN + 17, BatConfig::FINAL_IMAGE_TOP_MARGIN - 25, 40, 20), Qt::AlignHCenter, "dB");

    const double dbRange = BatConfig::MAX_DB - BatConfig::MIN_DB;
    const int dbStep = calculateBestDbStep(dbRange, colorBarArea.height());
    if (dbRange > 0 && dbStep > 0) {
        for (double db = BatConfig::MAX_DB; db >= BatConfig::MIN_DB; db -= dbStep) {
            int y = colorBarArea.top() + static_cast<int>(((BatConfig::MAX_DB - db) / dbRange) * colorBarArea.height());
            painter.drawText(QRect(colorBarArea.right() + 5, y - 10, 40, 20), Qt::AlignLeft | Qt::AlignVCenter, QString::number(db, 'f', 0));}}

    const double audioDuration = audioInfo.durationSec;
    if (audioDuration > 0) {
        const std::vector<int> tickLevels = { 3600, 1800, 900, 600, 300, 120, 60, 30, 15, 10, 5, 2, 1 };
        const int minHorizontalSpacing = 60;
        double pixelsPerSec = static_cast<double>(spectrogramWidth) / audioDuration;
        int timeStep = tickLevels.back();
        for (const int level : tickLevels) {
            if (level * pixelsPerSec >= minHorizontalSpacing) { timeStep = level; }
            else { break; }}

        QString totalTimeLabel = BatUtils::formatPreciseDuration(audioDuration);
        int totalLabelWidth = fm.horizontalAdvance(totalTimeLabel);
        const int padding = 5;
        int centerWidth = totalLabelWidth + 2 * padding;
        int totalTimeX = spectrogramOffsetX + spectrogramWidth;
        QRect totalRect(totalTimeX - centerWidth / 2, finalHeight - BatConfig::FINAL_IMAGE_BOTTOM_MARGIN + 5, centerWidth, 20);
        painter.drawText(totalRect, Qt::AlignHCenter | Qt::AlignTop, totalTimeLabel);

        if (settings.enableGrid) {
            painter.save();
            painter.translate(0.5, 0.5);
            QPen gridPen(Qt::white, 1, Qt::DotLine);
            painter.setPen(gridPen);

            for (int khz = freqStep; khz <= static_cast<int>(maxFreqKhz); khz += freqStep) {
                double linear_ratio = (maxFreqKhz > 0) ? (static_cast<double>(khz) / maxFreqKhz) : 0.0;
                double curved_ratio = MappingCurves::map(linear_ratio, settings.curveType);
                int y_pos = BatConfig::FINAL_IMAGE_TOP_MARGIN + static_cast<int>((1.0 - curved_ratio) * imageToProcess.height());

                painter.drawLine(spectrogramOffsetX, y_pos, spectrogramOffsetX + spectrogramWidth - 1, y_pos);}

            for (long long t = timeStep; t < audioDuration; t += timeStep) {
                int x_pos = spectrogramOffsetX + static_cast<int>(t * pixelsPerSec);
                painter.drawLine(x_pos, BatConfig::FINAL_IMAGE_TOP_MARGIN, x_pos, finalHeight - BatConfig::FINAL_IMAGE_BOTTOM_MARGIN - 1);}
            painter.restore();}

        painter.setPen(Qt::white);
        for (long long t = timeStep; t < audioDuration; t += timeStep) {
            int x_pos = spectrogramOffsetX + static_cast<int>(t * pixelsPerSec);
            QString timeLabel = QDateTime::fromSecsSinceEpoch(t, Qt::UTC).toString(t >= 3600 ? "h:mm:ss" : "m:ss");
            int labelWidth = fm.horizontalAdvance(timeLabel);
            int currentCenterWidth = labelWidth + 2 * padding;
            QRect labelRect(x_pos - currentCenterWidth / 2, finalHeight - BatConfig::FINAL_IMAGE_BOTTOM_MARGIN + 5, currentCenterWidth, 20);

            if (labelRect.intersects(totalRect)) { continue; }
            painter.drawText(labelRect, Qt::AlignHCenter | Qt::AlignTop, timeLabel);}}

    painter.end();

    std::unique_ptr<IImageEncoder> encoder = ImageEncoderFactory::createEncoder(settings.exportFormat);
    if (!encoder) {
        return false;}
    return encoder->encodeAndSave(finalImage, outputFilePath, settings.qualityLevel);
}


int BatImageExporter::calculateBestFreqStep(double maxFreqKhz, int availableHeight) const {
    if (maxFreqKhz <= 0 || availableHeight <= 0) return 1;
    const int minVerticalSpacing = 25;
    int numLabelsPossible = availableHeight / minVerticalSpacing;
    if (numLabelsPossible <= 1) return std::max(1, static_cast<int>(maxFreqKhz));
    double desiredStep = maxFreqKhz / numLabelsPossible;
    if (desiredStep <= 0) return 1;
    double powerOf10 = std::pow(10, std::floor(std::log10(desiredStep)));
    if (powerOf10 < 1) powerOf10 = 1;
    double normalizedStep = desiredStep / powerOf10;
    int finalStep;
    if (normalizedStep < 1.5)              finalStep = 1 * powerOf10;
        else if (normalizedStep < 3.5)     finalStep = 2 * powerOf10;
            else if (normalizedStep < 7.5) finalStep = 5 * powerOf10;
                else finalStep = 10 * powerOf10;
    return std::max(1, finalStep);
}


int BatImageExporter::calculateBestDbStep(double dbRange, int availableHeight) const {
    if (dbRange <= 0 || availableHeight <= 0) return 20;
    const int minVerticalSpacing = 30;
    int numLabelsPossible = availableHeight / minVerticalSpacing;
    if (numLabelsPossible <= 1) return std::max(1, static_cast<int>(dbRange));
    double desiredStep = dbRange / numLabelsPossible;
    double powerOf10 = std::pow(10, std::floor(std::log10(desiredStep)));
    if (powerOf10 <= 0) powerOf10 = 1;
    double normalizedStep = desiredStep / powerOf10;
    int finalStep;
    if (normalizedStep < 1.5) finalStep      = 1 * powerOf10;
    else if (normalizedStep < 2.2) finalStep = 2 * powerOf10;
    else if (normalizedStep < 3.5) finalStep = static_cast<int>(2.5 * powerOf10);
    else if (normalizedStep < 7.5) finalStep = 5 * powerOf10;
    else finalStep = 10 * powerOf10;
    return std::max(1, finalStep);
}
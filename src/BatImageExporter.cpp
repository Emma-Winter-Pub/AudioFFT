#include "BatImageExporter.h"
#include "BatWidget.h"
#include "BatAudioDecoder.h"
#include "BatConfig.h"
#include "BatUtils.h"
#include "AppConfig.h"
#include "ImageEncoderFactory.h"
#include "IImageEncoder.h"
#include "MappingCurves.h"
#include "ColorPalette.h" 

#include <memory>
#include <QPainter>
#include <QFileInfo>
#include <QLinearGradient>
#include <QApplication>
#include <QFontMetrics>
#include <QDateTime>
#include <cmath>
#include <algorithm>
#include <vector>

BatImageExporter::BatImageExporter() {}

BatImageExporter::~BatImageExporter() {}

ExportStatus BatImageExporter::exportImage(
    const QImage& rawSpectrogram,
    const QString& outputFilePath,
    const QString& sourceFileName,
    const BatSettings& settings,
    const BatDecodedAudio& audioInfo,
    double minDb,
    double maxDb)
{
    QImage imageToProcess = rawSpectrogram;
    if (settings.enableWidthLimit && imageToProcess.width() > settings.maxWidth) {
        imageToProcess = imageToProcess.scaled(
            settings.maxWidth,
            imageToProcess.height(),
            Qt::IgnoreAspectRatio,
            Qt::FastTransformation
        );
    }
    int maxFormatDim = std::numeric_limits<int>::max();
    if (settings.exportFormat == "JPG") {
        maxFormatDim = BatConfig::JPEG_MAX_DIMENSION;
    } else if (settings.exportFormat == "WebP") {
        maxFormatDim = BatConfig::BAT_WEBP_MAX_DIMENSION;
    } else if (settings.exportFormat == "AVIF") {
        maxFormatDim = BatConfig::BAT_AVIF_MAX_DIMENSION;
    }
    int topMargin = 0;
    int bottomMargin = 0;
    int leftMargin = 0;
    int rightMargin = 0;
    if (settings.enableComponents) {
        topMargin = BatConfig::FINAL_IMAGE_TOP_MARGIN;
        bottomMargin = BatConfig::FINAL_IMAGE_BOTTOM_MARGIN;
        leftMargin = BatConfig::FINAL_IMAGE_LEFT_MARGIN;
        rightMargin = BatConfig::FINAL_IMAGE_RIGHT_MARGIN;
    }
    int allowedSpecW = maxFormatDim - (leftMargin + rightMargin);
    int allowedSpecH = maxFormatDim - (topMargin + bottomMargin);
    if (allowedSpecW <= 0 || allowedSpecH <= 0) return ExportStatus::Failure;
    bool isResizedDueToFormat = false;
    int finalSpecW = imageToProcess.width();
    int finalSpecH = imageToProcess.height();
    if (finalSpecW > allowedSpecW) { finalSpecW = allowedSpecW; isResizedDueToFormat = true; }
    if (finalSpecH > allowedSpecH) { finalSpecH = allowedSpecH; isResizedDueToFormat = true; }
    if (isResizedDueToFormat) {
        imageToProcess = imageToProcess.scaled(finalSpecW, finalSpecH, Qt::IgnoreAspectRatio, Qt::FastTransformation);
    }
    QFont titleFont = QApplication::font();
    titleFont.setPointSize(10);
    QFontMetrics fm(titleFont);
    int fileNameWidth = settings.enableComponents ? fm.horizontalAdvance(sourceFileName) : 0;
    int spectrogramWidth = imageToProcess.width();
    int contentWidth = std::max(fileNameWidth, spectrogramWidth);
    const int colorBarWidth = settings.enableComponents ? 15 : 0;
    const int finalWidth = contentWidth + leftMargin + rightMargin;
    const int finalHeight = imageToProcess.height() + topMargin + bottomMargin;
    QImage finalImage(finalWidth, finalHeight, QImage::Format_RGB32);
    finalImage.fill(QColor(59, 68, 83));
    QPainter painter(&finalImage);
    painter.setRenderHint(QPainter::Antialiasing);
    int spectrogramOffsetX = leftMargin + (contentWidth - spectrogramWidth) / 2;
    painter.drawImage(spectrogramOffsetX, topMargin, imageToProcess);
    const double maxFreq = static_cast<double>(audioInfo.sampleRate) / 2.0;
    const double maxFreqKhz = maxFreq / 1000.0;
    const int freqStep = calculateBestFreqStep(maxFreqKhz, imageToProcess.height());
    const double audioDuration = audioInfo.durationSec;
    if (settings.enableComponents) {
        painter.setPen(Qt::white);
        painter.setFont(titleFont);
        QRect titleRect(leftMargin, 0, contentWidth, topMargin);
        painter.drawText(titleRect, Qt::AlignVCenter | Qt::AlignLeft, sourceFileName);
        painter.drawText(QRect(0, topMargin - 25, leftMargin - 5, 20), Qt::AlignRight | Qt::AlignVCenter, "kHz");
        QFontMetrics fm_khz(painter.font());
        const int minLabelSpacing = fm_khz.height();
        int lastDrawnLabelY = -1000;
        for (int khz = 0; khz <= static_cast<int>(maxFreqKhz); khz += freqStep) {
            if (khz > maxFreqKhz) break;
            double linear_ratio = (maxFreqKhz > 0) ? (static_cast<double>(khz) / maxFreqKhz) : 0.0;
            double curved_ratio = MappingCurves::forward(linear_ratio, settings.curveType, maxFreq);
            int y_pos = topMargin + static_cast<int>((1.0 - curved_ratio) * imageToProcess.height());
            if (std::abs(y_pos - lastDrawnLabelY) < minLabelSpacing && khz > 0) continue;
            painter.drawText(QRect(0, y_pos - 10, leftMargin - 5, 20), Qt::AlignRight | Qt::AlignVCenter, QString::number(khz));
            lastDrawnLabelY = y_pos;
        }
    }
    if (settings.enableComponents) {
        QRect colorBarArea(finalWidth - rightMargin + 5, topMargin, colorBarWidth, imageToProcess.height());
        QLinearGradient gradient(colorBarArea.topLeft(), colorBarArea.bottomLeft());
        std::vector<QColor> colorMap = ColorPalette::getPalette(settings.paletteType);
        if (colorMap.size() < 256) colorMap = ColorPalette::getPalette(PaletteType::S00);
        for (size_t i = 0; i < colorMap.size(); ++i) {
            gradient.setColorAt(1.0 - (static_cast<double>(i) / (colorMap.size() - 1)), colorMap[i]);
        }
        painter.fillRect(colorBarArea, gradient);
        painter.drawText(QRect(finalWidth - rightMargin + 17, topMargin - 25, 40, 20), Qt::AlignHCenter, "dB");
        const double dbRange = maxDb - minDb;
        const int dbStep = calculateBestDbStep(dbRange, colorBarArea.height());
        if (dbRange > 0 && dbStep > 0) {
            double start_db = std::ceil(maxDb / dbStep) * dbStep;
            if (start_db > maxDb) start_db -= dbStep;
            for (double db = start_db; db >= minDb; db -= dbStep) {
                int y = colorBarArea.top() + static_cast<int>(((maxDb - db) / dbRange) * colorBarArea.height());
                if (y >= colorBarArea.top() - 10 && y <= colorBarArea.bottom() + 10) {
                    painter.drawText(QRect(colorBarArea.right() + 5, y - 10, 40, 20), Qt::AlignLeft | Qt::AlignVCenter, QString::number(db, 'f', 0));
                }
            }
        }
    }
    if (audioDuration > 0) {
        const std::vector<int> tickLevels = { 3600, 1800, 900, 600, 300, 120, 60, 30, 15, 10, 5, 2, 1 };
        const int minHorizontalSpacing = 60;
        double pixelsPerSec = static_cast<double>(spectrogramWidth) / audioDuration;
        int timeStep = tickLevels.back();
        for (const int level : tickLevels) {
            if (level * pixelsPerSec >= minHorizontalSpacing) { timeStep = level; } else { break; }
        }
        if (settings.enableGrid) {
            painter.save();
            painter.translate(0.5, 0.5);
            QPen gridPen(Qt::white, 1, Qt::DotLine);
            painter.setPen(gridPen);
            for (int khz = freqStep; khz <= static_cast<int>(maxFreqKhz); khz += freqStep) {
                if (khz == 0) continue; 
                double linear_ratio = (maxFreqKhz > 0) ? (static_cast<double>(khz) / maxFreqKhz) : 0.0;
                double curved_ratio = MappingCurves::forward(linear_ratio, settings.curveType, maxFreq);
                int y_pos = topMargin + static_cast<int>((1.0 - curved_ratio) * imageToProcess.height());
                painter.drawLine(spectrogramOffsetX, y_pos, spectrogramOffsetX + spectrogramWidth - 1, y_pos);
            }
            for (long long t = timeStep; t < audioDuration; t += timeStep) {
                int x_pos = spectrogramOffsetX + static_cast<int>(t * pixelsPerSec);
                painter.drawLine(x_pos, topMargin, x_pos, finalHeight - bottomMargin - 1);
            }
            painter.restore();
        }
        if (settings.enableComponents) {
            QString totalTimeLabel = BatUtils::formatPreciseDuration(audioDuration);
            int totalLabelWidth = fm.horizontalAdvance(totalTimeLabel);
            const int padding = 5;
            int centerWidth = totalLabelWidth + 2 * padding;
            int totalTimeX = spectrogramOffsetX + spectrogramWidth;
            QRect totalRect(totalTimeX - centerWidth / 2, finalHeight - bottomMargin + 5, centerWidth, 20);
            painter.setPen(Qt::white);
            painter.drawText(totalRect, Qt::AlignHCenter | Qt::AlignTop, totalTimeLabel);
            for (long long t = timeStep; t < audioDuration; t += timeStep) {
                int x_pos = spectrogramOffsetX + static_cast<int>(t * pixelsPerSec);
                QString timeLabel = QDateTime::fromSecsSinceEpoch(t, Qt::UTC).toString(t >= 3600 ? "h:mm:ss" : "m:ss");
                int labelWidth = fm.horizontalAdvance(timeLabel);
                int currentCenterWidth = labelWidth + 2 * padding;
                QRect labelRect(x_pos - currentCenterWidth / 2, finalHeight - bottomMargin + 5, currentCenterWidth, 20);
                if (labelRect.intersects(totalRect)) continue;
                painter.drawText(labelRect, Qt::AlignHCenter | Qt::AlignTop, timeLabel);
            }
        }
    }
    painter.end();
    std::unique_ptr<IImageEncoder> encoder = ImageEncoderFactory::createEncoder(settings.exportFormat);
    if (!encoder) return ExportStatus::Failure;
    bool success = encoder->encodeAndSave(finalImage, outputFilePath, settings.qualityLevel);
    if (!success) return ExportStatus::Failure;
    return isResizedDueToFormat ? ExportStatus::SuccessWithResize : ExportStatus::Success;
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
    if (normalizedStep < 1.5) finalStep = 1 * powerOf10;
    else if (normalizedStep < 3.5) finalStep = 2 * powerOf10;
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
    if (normalizedStep < 1.5) finalStep = 1 * powerOf10;
    else if (normalizedStep < 2.2) finalStep = 2 * powerOf10;
    else if (normalizedStep < 3.5) finalStep = static_cast<int>(2.5 * powerOf10);
    else if (normalizedStep < 7.5) finalStep = 5 * powerOf10;
    else finalStep = 10 * powerOf10;
    return std::max(1, finalStep);
}
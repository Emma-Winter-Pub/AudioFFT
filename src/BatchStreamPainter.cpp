#include "BatchStreamPainter.h"
#include "AppConfig.h" 
#include "BatchStreamUtils.h"

#include <QPainter>
#include <QLinearGradient>
#include <QFontMetrics>
#include <QApplication>
#include <QFileInfo> 
#include <cmath>
#include <algorithm>
#include <mutex> 

static std::mutex s_painterMutex;

QImage BatchStreamPainter::drawFinalImage(
    const QImage& rawSpectrogram,
    const BatchStreamAudioInfo& info,
    const BatchStreamSettings& settings
) {
    std::lock_guard<std::mutex> lock(s_painterMutex);
    const int topMargin = settings.enableComponents ? AppConfig::PNG_SAVE_TOP_MARGIN : 0;
    const int bottomMargin = settings.enableComponents ? AppConfig::PNG_SAVE_BOTTOM_MARGIN : 0;
    const int leftMargin = settings.enableComponents ? AppConfig::PNG_SAVE_LEFT_MARGIN : 0;
    const int rightMargin = settings.enableComponents ? AppConfig::PNG_SAVE_RIGHT_MARGIN : 0;
    const int colorBarWidth = settings.enableComponents ? AppConfig::PNG_SAVE_COLOR_BAR_WIDTH : 0;
    QString fileName = QFileInfo(info.filePath).fileName();
    QFont titleFont = QApplication::font();
    titleFont.setPointSize(10);
    QFontMetrics fm(titleFont);
    int fileNameWidth = settings.enableComponents ? fm.horizontalAdvance(fileName) : 0;
    int spectrogramWidth = rawSpectrogram.width();
    int contentWidth = std::max(fileNameWidth, spectrogramWidth);
    int finalWidth = contentWidth + leftMargin + rightMargin;
    int finalHeight = rawSpectrogram.height() + topMargin + bottomMargin;
    QImage finalImage(finalWidth, finalHeight, QImage::Format_RGB32);
    finalImage.fill(QColor(59, 68, 83));
    QPainter painter(&finalImage);
    painter.setRenderHint(QPainter::Antialiasing);
    int spectrogramOffsetX = leftMargin + (contentWidth - spectrogramWidth) / 2;
    painter.drawImage(spectrogramOffsetX, topMargin, rawSpectrogram);
    const double maxFreq = static_cast<double>(info.sampleRate) / 2.0;
    const double maxFreqKhz = maxFreq / 1000.0;
    const int freqStep = calculateBestFreqStep(maxFreqKhz, rawSpectrogram.height());
    if (settings.enableComponents) {
        painter.setPen(Qt::white);
        painter.setFont(titleFont);
        QRect titleRect(leftMargin, 0, contentWidth, topMargin);
        painter.drawText(titleRect, Qt::AlignVCenter | Qt::AlignLeft, fileName);
        painter.drawText(QRect(0, topMargin - 25, leftMargin - 5, 20), Qt::AlignRight | Qt::AlignVCenter, "kHz");
        QFontMetrics fm_khz(painter.font());
        const int minLabelSpacing = fm_khz.height();
        int lastDrawnLabelY = -1000;
        for (int khz = 0; khz <= static_cast<int>(maxFreqKhz); khz += freqStep) {
            if (khz > maxFreqKhz) break;
            double linear_ratio = (maxFreqKhz > 0) ? (static_cast<double>(khz) / maxFreqKhz) : 0.0;
            double curved_ratio = MappingCurves::forward(linear_ratio, settings.curveType, maxFreq);
            int y_pos = topMargin + static_cast<int>((1.0 - curved_ratio) * rawSpectrogram.height());
            if (std::abs(y_pos - lastDrawnLabelY) >= minLabelSpacing || khz == 0) {
                painter.drawText(QRect(0, y_pos - 10, leftMargin - 5, 20), Qt::AlignRight | Qt::AlignVCenter, QString::number(khz));
                lastDrawnLabelY = y_pos;
            }
        }
    }
    if (settings.enableGrid) {
        painter.save();
        painter.translate(0.5, 0.5);
        QPen gridPen(AppConfig::PNG_SAVE_GRID_LINE_COLOR, 1, Qt::DotLine);
        painter.setPen(gridPen);
        for (int khz = freqStep; khz <= static_cast<int>(maxFreqKhz); khz += freqStep) {
            double linear_ratio = (maxFreqKhz > 0) ? (static_cast<double>(khz) / maxFreqKhz) : 0.0;
            double curved_ratio = MappingCurves::forward(linear_ratio, settings.curveType, maxFreq);
            int y_pos = topMargin + static_cast<int>((1.0 - curved_ratio) * rawSpectrogram.height());
            painter.drawLine(spectrogramOffsetX, y_pos, spectrogramOffsetX + spectrogramWidth - 1, y_pos);
        }
        painter.restore();
    }
    if (settings.enableComponents) {
        QRect colorBarArea(finalWidth - rightMargin + 5, topMargin, colorBarWidth, rawSpectrogram.height());
        QLinearGradient gradient(colorBarArea.topLeft(), colorBarArea.bottomLeft());
        std::vector<QColor> colorMap = ColorPalette::getPalette(settings.paletteType);
        if (colorMap.size() < 256) colorMap = ColorPalette::getPalette(PaletteType::S00);
        for (size_t i = 0; i < colorMap.size(); ++i) {
            gradient.setColorAt(1.0 - (static_cast<double>(i) / (colorMap.size() - 1)), colorMap[i]);
        }
        painter.fillRect(colorBarArea, gradient);
        painter.drawText(QRect(finalWidth - rightMargin + 17, topMargin - 25, 40, 20), Qt::AlignHCenter, "dB");
        const double dbRange = settings.maxDb - settings.minDb;
        const int dbStep = calculateBestDbStep(dbRange, colorBarArea.height());
        if (dbRange > 0 && dbStep > 0) {
            double start_db = std::ceil(settings.maxDb / dbStep) * dbStep;
            if (start_db > settings.maxDb) start_db -= dbStep;
            for (double db = start_db; db >= settings.minDb; db -= dbStep) {
                int y = colorBarArea.top() + static_cast<int>(((settings.maxDb - db) / dbRange) * colorBarArea.height());
                if (y >= colorBarArea.top() - 10 && y <= colorBarArea.bottom() + 10) {
                    painter.drawText(QRect(colorBarArea.right() + 5, y - 10, 40, 20), Qt::AlignLeft | Qt::AlignVCenter, QString::number(db, 'f', 0));
                }
            }
        }
    }
    if (info.durationSec > 0) {
        const std::vector<int> tickLevels = { 3600, 1800, 900, 600, 300, 120, 60, 30, 15, 10, 5, 2, 1 };
        const int minHorizontalSpacing = 60;
        double pixelsPerSec = static_cast<double>(spectrogramWidth) / info.durationSec;
        int timeStep = tickLevels.back();
        for (const int level : tickLevels) {
            if (level * pixelsPerSec >= minHorizontalSpacing) { timeStep = level; }
            else { break; }
        }
        if (settings.enableGrid) {
            painter.save();
            painter.translate(0.5, 0.5);
            QPen gridPen(AppConfig::PNG_SAVE_GRID_LINE_COLOR, 1, Qt::DotLine);
            painter.setPen(gridPen);
            for (long long t = timeStep; t < info.durationSec; t += timeStep) {
                int x_pos = spectrogramOffsetX + static_cast<int>(t * pixelsPerSec);
                painter.drawLine(x_pos, topMargin, x_pos, finalHeight - bottomMargin - 1);
            }
            painter.restore();
        }
        if (settings.enableComponents) {
            QString totalTimeLabel = BatchStreamUtils::formatPreciseDuration(info.durationSec);
            int totalLabelWidth = fm.horizontalAdvance(totalTimeLabel);
            const int padding = 5;
            int centerWidth = totalLabelWidth + 2 * padding;
            int totalTimeX = spectrogramOffsetX + spectrogramWidth;
            QRect totalRect(totalTimeX - centerWidth / 2, finalHeight - bottomMargin + 5, centerWidth, 20);
            painter.drawText(totalRect, Qt::AlignHCenter | Qt::AlignTop, totalTimeLabel);
            for (long long t = timeStep; t < info.durationSec; t += timeStep) {
                int x_pos = spectrogramOffsetX + static_cast<int>(t * pixelsPerSec);
                QString timeLabel = BatchStreamUtils::formatDuration(t);
                int labelWidth = fm.horizontalAdvance(timeLabel);
                int currentCenterWidth = labelWidth + 2 * padding;
                QRect labelRect(x_pos - currentCenterWidth / 2, finalHeight - bottomMargin + 5, currentCenterWidth, 20);
                if (!labelRect.intersects(totalRect)) {
                    painter.drawText(labelRect, Qt::AlignHCenter | Qt::AlignTop, timeLabel);
                }
            }
        }
    }
    painter.end();
    return finalImage;
}

int BatchStreamPainter::calculateBestFreqStep(double maxFreqKhz, int availableHeight) {
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

int BatchStreamPainter::calculateBestDbStep(double dbRange, int availableHeight) {
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
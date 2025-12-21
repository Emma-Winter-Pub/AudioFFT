#include "SpectrogramPainter.h"
#include "AppConfig.h"
#include "MappingCurves.h"

#include <cmath>
#include <vector>
#include <algorithm>
#include <QThread>
#include <QPainter>
#include <QDateTime>
#include <QtConcurrent>
#include <QFontMetrics>
#include <QLinearGradient>


int SpectrogramPainter::calculateBestFreqStepPng(double maxFreqKhz, int availableHeight) {
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

int SpectrogramPainter::calculateBestDbStep(double dbRange, int availableHeight) {
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


SpectrogramPainter::SpectrogramPainter(QObject *parent) : QObject(parent) {}


QImage SpectrogramPainter::drawFinalImage(
    const QImage &spectrogramToSave,
    const QString &fileName,
    double audioDuration,
    bool showGrid,
    const QString &preciseDurationStr,
    int nativeSampleRate,
    CurveType curveType)
{
    bool useMultiThreading = (spectrogramToSave.width() > 10000 || spectrogramToSave.height() >= 3000);

    if (useMultiThreading) {
        return createFinalImage_MultiThread(
            spectrogramToSave, fileName, audioDuration, showGrid, preciseDurationStr, nativeSampleRate, curveType
        );
    } else {
        return createFinalImage_SingleThread(
            spectrogramToSave, fileName, audioDuration, showGrid, preciseDurationStr, nativeSampleRate, curveType
        );
    }
}


QImage SpectrogramPainter::createFinalImage_MultiThread(const QImage& rawSpectrogram, const QString& fileName, double audioDuration, bool showGrid, const QString& preciseDurationStr, int nativeSampleRate, CurveType curveType){

    int numThreads = QThread::idealThreadCount();
    
    const int topMargin = AppConfig::PNG_SAVE_TOP_MARGIN;
    const int bottomMargin = AppConfig::PNG_SAVE_BOTTOM_MARGIN;
    const int leftMargin = AppConfig::PNG_SAVE_LEFT_MARGIN;
    const int rightMargin = AppConfig::PNG_SAVE_RIGHT_MARGIN;
    const int colorBarWidth = AppConfig::PNG_SAVE_COLOR_BAR_WIDTH;

    QFont titleFont("", 10);
    QFontMetrics fm(titleFont);
    int fileNameWidth = fm.horizontalAdvance(fileName);
    int spectrogramWidth = rawSpectrogram.width();
    int contentWidth = std::max(fileNameWidth, spectrogramWidth);

    int finalWidth = contentWidth + leftMargin + rightMargin;
    int finalHeight = rawSpectrogram.height() + topMargin + bottomMargin;

    QImage finalImage(finalWidth, finalHeight, QImage::Format_RGB32);
    finalImage.fill(QColor(59, 68, 83));

    const int spectrogramHeight = rawSpectrogram.height();
    int stripHeight = spectrogramHeight / numThreads;
    const double maxFreqKhz = (nativeSampleRate / 2.0) / 1000.0;
    const int freqStep = calculateBestFreqStepPng(maxFreqKhz, rawSpectrogram.height());
    
    int spectrogramOffsetX = leftMargin + (contentWidth - spectrogramWidth) / 2;

    auto renderStrip = [&](int threadIndex) -> QImage {
        int y_start = threadIndex * stripHeight;
        int y_end = (threadIndex == numThreads - 1) ? spectrogramHeight : y_start + stripHeight;
        int currentStripHeight = y_end - y_start;

        if (currentStripHeight <= 0) return QImage();

        QImage stripImage(spectrogramWidth, currentStripHeight, QImage::Format_RGB32);
        QPainter p(&stripImage);

        QRect sourceRect(0, y_start, spectrogramWidth, currentStripHeight);
        QRect destRect(0, 0, spectrogramWidth, currentStripHeight);
        p.drawImage(destRect, rawSpectrogram, sourceRect);
        
        if (showGrid) {
            p.save();
            p.translate(0.5, 0.5);
            QPen gridPen(AppConfig::PNG_SAVE_GRID_LINE_COLOR, 1, Qt::DotLine);
            p.setPen(gridPen);
            
            if (audioDuration > 0) {
                 const std::vector<int> tickLevels = { 3600, 1800, 900, 600, 300, 120, 60, 30, 15, 10, 5, 2, 1 };
                 const int minHorizontalSpacing = 50;
                 double pixelsPerSec = spectrogramWidth / audioDuration;
                 int timeStep = tickLevels.back();
                 for(const int level : tickLevels) { if(level * pixelsPerSec >= minHorizontalSpacing) timeStep = level; else break; }

                 for (long long t = timeStep; t < audioDuration; t += timeStep) {
                    int x_pos = static_cast<int>(t * pixelsPerSec);
                    p.drawLine(x_pos, 0, x_pos, currentStripHeight -1);}}
            p.restore();}
        return stripImage;};
    
    QList<int> threadIndices;
    for(int i=0; i<numThreads; ++i) threadIndices << i;
    QList<QImage> renderedStrips = QtConcurrent::blockingMapped(threadIndices, renderStrip);

    QPainter finalPainter(&finalImage);
    int current_y = topMargin;
    for(const QImage& strip : renderedStrips){
        if(!strip.isNull()){
            finalPainter.drawImage(spectrogramOffsetX, current_y, strip);
            current_y += strip.height();}}

    finalPainter.setRenderHint(QPainter::Antialiasing);
    finalPainter.setPen(AppConfig::TEXT_COLOR);
    finalPainter.setFont(titleFont);

    QRect titleRect(leftMargin, 0, contentWidth, topMargin);
    finalPainter.drawText(titleRect, Qt::AlignVCenter | Qt::AlignLeft, fileName);

    finalPainter.drawText(QRect(0, topMargin - 25, leftMargin - 5, 20), Qt::AlignRight | Qt::AlignVCenter, "kHz");

    QFontMetrics fm_khz(finalPainter.font());
    const int minLabelSpacing = fm_khz.height();
    int lastDrawnLabelY = -1000;

    for (int khz = 0; khz <= static_cast<int>(maxFreqKhz); khz += freqStep) {
        if (khz > maxFreqKhz) break;
        
        double linear_ratio = (maxFreqKhz > 0) ? (static_cast<double>(khz) / maxFreqKhz) : 0.0;
        double curved_ratio = MappingCurves::map(linear_ratio, curveType);
        int y_pos = topMargin + static_cast<int>((1.0 - curved_ratio) * rawSpectrogram.height());
        
        bool shouldDrawLabel = (std::abs(y_pos - lastDrawnLabelY) >= minLabelSpacing) || (khz == 0);
        
        if (shouldDrawLabel) {
            finalPainter.drawText(QRect(0, y_pos - 10, leftMargin - 5, 20), Qt::AlignRight | Qt::AlignVCenter, QString::number(khz));
            lastDrawnLabelY = y_pos;}

        if (showGrid && khz > 0) {
            finalPainter.save();
            finalPainter.translate(0.5, 0.5);
            QPen gridPen(AppConfig::PNG_SAVE_GRID_LINE_COLOR, 1, Qt::DotLine);
            finalPainter.setPen(gridPen);
            finalPainter.drawLine(spectrogramOffsetX, y_pos, spectrogramOffsetX + spectrogramWidth - 1, y_pos);
            finalPainter.restore();}}

    QRect colorBarArea(finalWidth - rightMargin + 5, topMargin, colorBarWidth, rawSpectrogram.height());
    QLinearGradient gradient(colorBarArea.topLeft(), colorBarArea.bottomLeft());
    auto colorMap = AppConfig::getColorMap();
    for (size_t i = 0; i < colorMap.size(); ++i) {
        gradient.setColorAt(1.0 - (static_cast<double>(i) / (colorMap.size() - 1)), colorMap[i]);}

    finalPainter.fillRect(colorBarArea, gradient);
    finalPainter.drawText(QRect(finalWidth - rightMargin + 17, topMargin - 25, 40, 20), Qt::AlignHCenter, "dB");

    const double dbRange = AppConfig::MAX_DB - AppConfig::MIN_DB;
    const int dbStep = calculateBestDbStep(dbRange, colorBarArea.height());
    if (dbRange > 0 && dbStep > 0) {
        double start_db = std::ceil(AppConfig::MAX_DB / dbStep) * dbStep;
        for (double db = start_db; db >= AppConfig::MIN_DB; db -= dbStep) {
            int y = colorBarArea.top() + static_cast<int>(((AppConfig::MAX_DB - db) / dbRange) * colorBarArea.height());
            finalPainter.drawText(QRect(colorBarArea.right() + 5, y - 10, 40, 20), Qt::AlignLeft | Qt::AlignVCenter, QString::number(db, 'f', 0));}}

    if (audioDuration > 0) {
        const std::vector<int> tickLevels = { 3600, 1800, 900, 600, 300, 120, 60, 30, 15, 10, 5, 2, 1 };
        const int minHorizontalSpacing = 60;
        double pixelsPerSec = static_cast<double>(spectrogramWidth) / audioDuration;
        int timeStep = tickLevels.back();
        for (const int level : tickLevels) {
            if (level * pixelsPerSec >= minHorizontalSpacing) { timeStep = level; }
            else { break; }}

        QString totalTimeLabel = preciseDurationStr.isEmpty() ?
            QDateTime::fromSecsSinceEpoch(static_cast<qint64>(audioDuration), Qt::UTC).toString(audioDuration >= 3600 ? "h:mm:ss" : "m:ss") :
            preciseDurationStr;
        int totalLabelWidth = fm.horizontalAdvance(totalTimeLabel);
        const int padding = 5;
        int centerWidth = totalLabelWidth + 2 * padding;
        int totalTimeX = spectrogramOffsetX + spectrogramWidth;
        QRect totalRect(totalTimeX - centerWidth / 2, finalHeight - bottomMargin + 5, centerWidth, 20);

        for (long long t = timeStep; t < audioDuration; t += timeStep) {
            int x_pos = spectrogramOffsetX + static_cast<int>(t * pixelsPerSec);
            QString timeLabel = QDateTime::fromSecsSinceEpoch(t, Qt::UTC).toString(t >= 3600 ? "h:mm:ss" : "m:ss");
            int labelWidth = fm.horizontalAdvance(timeLabel);
            int currentCenterWidth = labelWidth + 2 * padding;
            QRect labelRect(x_pos - currentCenterWidth / 2, finalHeight - bottomMargin + 5, currentCenterWidth, 20);
            if (labelRect.intersects(totalRect)) { continue; }
            finalPainter.drawText(labelRect, Qt::AlignHCenter | Qt::AlignTop, timeLabel);}

        finalPainter.drawText(totalRect, Qt::AlignHCenter | Qt::AlignTop, totalTimeLabel);
    }

    finalPainter.end();
    return finalImage;
}


QImage SpectrogramPainter::createFinalImage_SingleThread(const QImage& rawSpectrogram, const QString& fileName, double audioDuration, bool showGrid, const QString& preciseDurationStr, int nativeSampleRate, CurveType curveType){
    const int topMargin = AppConfig::PNG_SAVE_TOP_MARGIN;
    const int bottomMargin = AppConfig::PNG_SAVE_BOTTOM_MARGIN;
    const int leftMargin = AppConfig::PNG_SAVE_LEFT_MARGIN;
    const int rightMargin = AppConfig::PNG_SAVE_RIGHT_MARGIN;
    const int colorBarWidth = AppConfig::PNG_SAVE_COLOR_BAR_WIDTH;

    QFont titleFont("", 10);
    QFontMetrics fm(titleFont);
    int fileNameWidth = fm.horizontalAdvance(fileName);
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
    
    painter.setPen(AppConfig::TEXT_COLOR);
    painter.setFont(titleFont);
    
    QRect titleRect(leftMargin, 0, contentWidth, topMargin);
    painter.drawText(titleRect, Qt::AlignVCenter | Qt::AlignLeft, fileName);
    
    const double maxFreqKhz = (nativeSampleRate / 2.0) / 1000.0;
    const int freqStep = calculateBestFreqStepPng(maxFreqKhz, rawSpectrogram.height());
    
    painter.drawText(QRect(0, topMargin - 25, leftMargin - 5, 20), Qt::AlignRight | Qt::AlignVCenter, "kHz");

    QFontMetrics fm_khz(painter.font());
    const int minLabelSpacing = fm_khz.height();
    int lastDrawnLabelY = -1000;

    for (int khz = 0; khz <= static_cast<int>(maxFreqKhz); khz += freqStep) {
        if (khz > maxFreqKhz) break;

        double linear_ratio = (maxFreqKhz > 0) ? (static_cast<double>(khz) / maxFreqKhz) : 0.0;
        double curved_ratio = MappingCurves::map(linear_ratio, curveType);
        int y_pos = topMargin + static_cast<int>((1.0 - curved_ratio) * rawSpectrogram.height());
        
        bool shouldDrawLabel = (std::abs(y_pos - lastDrawnLabelY) >= minLabelSpacing) || (khz == 0);

        if (shouldDrawLabel) {
            painter.drawText(QRect(0, y_pos - 10, leftMargin - 5, 20), Qt::AlignRight | Qt::AlignVCenter, QString::number(khz));
            lastDrawnLabelY = y_pos;}}
    
    QRect colorBarArea(finalWidth - rightMargin + 5, topMargin, colorBarWidth, rawSpectrogram.height());
    QLinearGradient gradient(colorBarArea.topLeft(), colorBarArea.bottomLeft());
    auto colorMap = AppConfig::getColorMap();
    for (size_t i = 0; i < colorMap.size(); ++i) {
        gradient.setColorAt(1.0 - (static_cast<double>(i) / (colorMap.size() - 1)), colorMap[i]);}

    painter.fillRect(colorBarArea, gradient);
    
    painter.drawText(QRect(finalWidth - rightMargin + 17, topMargin - 25, 40, 20), Qt::AlignHCenter, "dB");
    
    const double dbRange = AppConfig::MAX_DB - AppConfig::MIN_DB;
    const int dbStep = calculateBestDbStep(dbRange, colorBarArea.height());
    if (dbRange > 0 && dbStep > 0) {
        double start_db = std::ceil(AppConfig::MAX_DB / dbStep) * dbStep;
        for (double db = start_db; db >= AppConfig::MIN_DB; db -= dbStep) {
            int y = colorBarArea.top() + static_cast<int>(((AppConfig::MAX_DB - db) / dbRange) * colorBarArea.height());
            painter.drawText(QRect(colorBarArea.right() + 5, y - 10, 40, 20), Qt::AlignLeft | Qt::AlignVCenter, QString::number(db, 'f', 0));}}
    
    if (audioDuration > 0) {
        const std::vector<int> tickLevels = { 3600, 1800, 900, 600, 300, 120, 60, 30, 15, 10, 5, 2, 1 };
        const int minHorizontalSpacing = 60;
        double pixelsPerSec = static_cast<double>(spectrogramWidth) / audioDuration;
        int timeStep = tickLevels.back();
        for (const int level : tickLevels) {
            if (level * pixelsPerSec >= minHorizontalSpacing) { timeStep = level; } else { break; }}
        
        QString totalTimeLabel = preciseDurationStr.isEmpty() ?
            QDateTime::fromSecsSinceEpoch(static_cast<qint64>(audioDuration), Qt::UTC).toString(audioDuration >= 3600 ? "h:mm:ss" : "m:ss") :
            preciseDurationStr;
        int totalLabelWidth = fm.horizontalAdvance(totalTimeLabel);
        const int padding = 5;
        int centerWidth = totalLabelWidth + 2 * padding;
        int totalTimeX = spectrogramOffsetX + spectrogramWidth;
        QRect totalRect(totalTimeX - centerWidth / 2, finalHeight - bottomMargin + 5, centerWidth, 20);
        
        for (long long t = timeStep; t < audioDuration; t += timeStep) {
            int x_pos = spectrogramOffsetX + static_cast<int>(t * pixelsPerSec);
            QString timeLabel = QDateTime::fromSecsSinceEpoch(t, Qt::UTC).toString(t >= 3600 ? "h:mm:ss" : "m:ss");
            int labelWidth = fm.horizontalAdvance(timeLabel);
            int currentCenterWidth = labelWidth + 2 * padding;
            QRect labelRect(x_pos - currentCenterWidth / 2, finalHeight - bottomMargin + 5, currentCenterWidth, 20);
            if (labelRect.intersects(totalRect)) { continue; }
            painter.drawText(labelRect, Qt::AlignHCenter | Qt::AlignTop, timeLabel);}

        painter.drawText(totalRect, Qt::AlignHCenter | Qt::AlignTop, totalTimeLabel);
        
        if (showGrid) {
            painter.save();
            painter.translate(0.5, 0.5);
            QPen gridPen(AppConfig::PNG_SAVE_GRID_LINE_COLOR, 1, Qt::DotLine);
            painter.setPen(gridPen);
            
            for (int khz = freqStep; khz <= static_cast<int>(maxFreqKhz); khz += freqStep) {
                if(khz > maxFreqKhz) break;
                double linear_ratio = (maxFreqKhz > 0) ? (static_cast<double>(khz) / maxFreqKhz) : 0.0;
                double curved_ratio = MappingCurves::map(linear_ratio, curveType);
                int y_pos = topMargin + static_cast<int>((1.0 - curved_ratio) * rawSpectrogram.height());
                painter.drawLine(spectrogramOffsetX, y_pos, spectrogramOffsetX + spectrogramWidth - 1, y_pos);}

            for (long long t = timeStep; t < audioDuration; t += timeStep) {
                int x_pos = spectrogramOffsetX + static_cast<int>(t * pixelsPerSec);
                painter.drawLine(x_pos, topMargin, x_pos, finalHeight - bottomMargin - 1);}
            painter.restore();}}
    
    painter.end();
    return finalImage;
}
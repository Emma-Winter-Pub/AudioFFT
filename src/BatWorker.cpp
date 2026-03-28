#include "BatWorker.h"
#include "BatProcessor.h"
#include "BatAudioDecoder.h"
#include "BatFftProcessor.h"
#include "BatSpectrogramGenerator.h"
#include "BatUtils.h"
#include "ImageEncoderFactory.h"
#include "AppConfig.h"
#include "BatConfig.h"
#include "MappingCurves.h"
#include "ColorPalette.h"

#include <QFileInfo>
#include <QDir>
#include <QPainter>
#include <QLinearGradient>
#include <QFontMetrics>
#include <QApplication>
#include <QDateTime>
#include <QElapsedTimer>
#include <QThread>
#include <QFile>
#include <algorithm>
#include <cmath>
#include <iostream>
#include <limits>
#include <cstring>

static int calculateBestFreqStep(double maxFreqKhz, int availableHeight) {
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

static int calculateBestDbStep(double dbRange, int availableHeight) {
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

static QImage drawComposedImage(const QImage& rawImage, const QString& fileName, double durationSec, int sampleRate, const BatSettings& settings) {
    int topMargin = 0, bottomMargin = 0, leftMargin = 0, rightMargin = 0;
    if (settings.enableComponents) {
        topMargin = BatConfig::FINAL_IMAGE_TOP_MARGIN;
        bottomMargin = BatConfig::FINAL_IMAGE_BOTTOM_MARGIN;
        leftMargin = BatConfig::FINAL_IMAGE_LEFT_MARGIN;
        rightMargin = BatConfig::FINAL_IMAGE_RIGHT_MARGIN;
    }
    QFont titleFont = QApplication::font();
    titleFont.setPointSize(10);
    QFontMetrics fm(titleFont);
    int fileNameWidth = settings.enableComponents ? fm.horizontalAdvance(fileName) : 0;
    int spectrogramWidth = rawImage.width();
    int contentWidth = std::max(fileNameWidth, spectrogramWidth);
    int finalWidth = contentWidth + leftMargin + rightMargin;
    int finalHeight = rawImage.height() + topMargin + bottomMargin;
    QImage finalImage(finalWidth, finalHeight, QImage::Format_RGB32);
    finalImage.fill(QColor(59, 68, 83));
    QPainter painter(&finalImage);
    painter.setRenderHint(QPainter::Antialiasing);
    int specOffsetX = leftMargin + (contentWidth - spectrogramWidth) / 2;
    painter.drawImage(specOffsetX, topMargin, rawImage);
    const double maxFreq = static_cast<double>(sampleRate) / 2.0;
    const double maxFreqKhz = maxFreq / 1000.0;
    const int freqStep = calculateBestFreqStep(maxFreqKhz, rawImage.height());
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
            int y_pos = topMargin + static_cast<int>((1.0 - curved_ratio) * rawImage.height());
            if (std::abs(y_pos - lastDrawnLabelY) >= minLabelSpacing || khz == 0) {
                painter.drawText(QRect(0, y_pos - 10, leftMargin - 5, 20), Qt::AlignRight | Qt::AlignVCenter, QString::number(khz));
                lastDrawnLabelY = y_pos;
            }
            if (settings.enableGrid && khz > 0) {
                painter.save();
                painter.translate(0.5, 0.5);
                QPen gridPen(Qt::white, 1, Qt::DotLine);
                painter.setPen(gridPen);
                painter.drawLine(specOffsetX, y_pos, specOffsetX + spectrogramWidth - 1, y_pos);
                painter.restore();
            }
        }
        const int colorBarWidth = 15;
        QRect colorBarArea(finalWidth - rightMargin + 5, topMargin, colorBarWidth, rawImage.height());
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
    if (durationSec > 0) {
        const std::vector<int> tickLevels = { 3600, 1800, 900, 600, 300, 120, 60, 30, 15, 10, 5, 2, 1 };
        const int minHorizontalSpacing = 60;
        double pixelsPerSec = static_cast<double>(spectrogramWidth) / durationSec;
        int timeStep = tickLevels.back();
        for (const int level : tickLevels) {
            if (level * pixelsPerSec >= minHorizontalSpacing) { timeStep = level; } 
            else { break; }
        }
        if (settings.enableComponents) {
            QString totalTimeLabel = BatUtils::formatPreciseDuration(durationSec);
            int totalLabelWidth = fm.horizontalAdvance(totalTimeLabel);
            const int padding = 5;
            int centerWidth = totalLabelWidth + 2 * padding;
            int totalTimeX = specOffsetX + spectrogramWidth;
            QRect totalRect(totalTimeX - centerWidth / 2, finalHeight - bottomMargin + 5, centerWidth, 20);
            painter.drawText(totalRect, Qt::AlignHCenter | Qt::AlignTop, totalTimeLabel);
            for (long long t = timeStep; t < durationSec; t += timeStep) {
                int x_pos = specOffsetX + static_cast<int>(t * pixelsPerSec);
                if (settings.enableGrid) {
                    painter.save();
                    painter.translate(0.5, 0.5);
                    QPen gridPen(Qt::white, 1, Qt::DotLine);
                    painter.setPen(gridPen);
                    painter.drawLine(x_pos, topMargin, x_pos, finalHeight - bottomMargin - 1);
                    painter.restore();
                }
                QString timeLabel = QDateTime::fromSecsSinceEpoch(t, Qt::UTC).toString(t >= 3600 ? "h:mm:ss" : "m:ss");
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

BatWorker::BatWorker(int bucketId, int bucketIdPadding, std::atomic<bool>* stopFlag, std::atomic<bool>* pauseFlag, BatProcessor* processor, QObject *parent)
    : QObject(parent),
      m_bucketId(bucketId),
      m_bucketIdPadding(bucketIdPadding),
      m_stopFlag(stopFlag),
      m_pauseFlag(pauseFlag),
      m_processor(processor)
{}

void BatWorker::startWorkLoop(const BatSettings& settings) {
    m_bucketTimer.start();
    while (true) {
        checkPauseState();
        if (m_stopFlag->load()) break;
        if (!m_processor) break;
        auto taskOpt = m_processor->claimNextTask();
        if (!taskOpt.has_value()) {
            break;
        }
        BatTask task = std::move(taskOpt.value());
        QString filePath = task.info.path;
        QString fileName = QFileInfo(filePath).fileName();
        QString sourceTag = task.isLoadedInMemory ? tr("[内存]") : tr("[磁盘]");
        emit logMessage(QString(tr("%1 [线程 %2] %3 开始：%4\n"))
            .arg(BatUtils::getCurrentTimestamp())
            .arg(m_bucketId, m_bucketIdPadding, 10, QChar('0'))
            .arg(sourceTag)
            .arg(fileName));
        QElapsedTimer fileTimer;
        fileTimer.start();
        QString errorMsg;
        bool isResized = false;
        FilePerformanceMetrics metrics;
        bool success = processSingleFile(task, settings, errorMsg, isResized, metrics);
        task.memoryData.reset();
        if (success) {
            QString successInfo = isResized ? "WARNING_RESIZED" : "";
            emit fileCompleted(filePath, true, successInfo, metrics, m_bucketId);
        } else {
            emit fileCompleted(filePath, false, errorMsg, metrics, m_bucketId);
        }
        m_processor->notifyLargeFileFinished();
    }
    qint64 elapsed = m_bucketTimer.elapsed();
    emit bucketFinished(m_bucketId, elapsed);
}

void BatWorker::checkPauseState() {
    while (m_pauseFlag->load() && !m_stopFlag->load()) {
        QThread::msleep(100);
    }
}

bool BatWorker::processSingleFile(BatTask& task, const BatSettings& settings, QString& errorMsg, bool& outIsResized, FilePerformanceMetrics& outMetrics)  {
    outIsResized = false;
    QString filePath = task.info.path;
    QString fileName = QFileInfo(filePath).fileName();
    QElapsedTimer perfTimer;
    perfTimer.start();
    BatAudioDecoder decoder;
    std::optional<BatDecodedAudio> decodedOpt;
    if (task.isLoadedInMemory && task.memoryData) {
        decodedOpt = decoder.decode(task.memoryData);
    } else {
        decodedOpt = decoder.decode(filePath);
    }
    qint64 tDecode = perfTimer.elapsed();
    if (!decodedOpt.has_value()) {
        errorMsg = tr("解码失败 可能是文件损坏或格式不支持");
        return false;
    }
    const auto& audioInfo = decodedOpt.value();
    perfTimer.restart();
    int fftSize = getRequiredFftSize(settings.imageHeight);
    double effectiveInterval = settings.timeInterval;
    if (effectiveInterval <= 0.000000001) {
        if (audioInfo.sampleRate > 0) {
            effectiveInterval = static_cast<double>(fftSize) / audioInfo.sampleRate;
        } else {
            effectiveInterval = 0.1;
        }
    }
    BatFftProcessor fftProcessor;
    auto spectrumOpt = fftProcessor.compute(
        audioInfo.pcmData, 
        effectiveInterval, 
        fftSize, 
        audioInfo.sampleRate, 
        settings.windowType
    );
    qint64 tFft = perfTimer.elapsed();
    if (!spectrumOpt.has_value()) {
        errorMsg = tr("FFT 计算失败 音频时长可能过短");
        return false;
    }
    perfTimer.restart(); 
    BatSpectrogramGenerator generator;
    QImage rawSpectrogram = generator.generate(
        spectrumOpt.value(), 
        fftSize, 
        settings.imageHeight, 
        audioInfo.sampleRate, 
        settings.curveType, 
        settings.minDb, 
        settings.maxDb, 
        settings.paletteType
    );
    if (rawSpectrogram.isNull()) {
        errorMsg = tr("频谱图生成失败 图像为空");
        return false;
    }
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
    if (settings.exportFormat == "JPG") maxFormatDim = BatConfig::JPEG_MAX_DIMENSION;
    else if (settings.exportFormat == "WebP") maxFormatDim = BatConfig::BAT_WEBP_MAX_DIMENSION;
    else if (settings.exportFormat == "AVIF") maxFormatDim = BatConfig::BAT_AVIF_MAX_DIMENSION;
    int topMargin = 0, bottomMargin = 0, leftMargin = 0, rightMargin = 0;
    if (settings.enableComponents) {
        topMargin = BatConfig::FINAL_IMAGE_TOP_MARGIN;
        bottomMargin = BatConfig::FINAL_IMAGE_BOTTOM_MARGIN;
        leftMargin = BatConfig::FINAL_IMAGE_LEFT_MARGIN;
        rightMargin = BatConfig::FINAL_IMAGE_RIGHT_MARGIN;
    }
    int allowedSpecW = maxFormatDim - (leftMargin + rightMargin);
    int allowedSpecH = maxFormatDim - (topMargin + bottomMargin);
    if (allowedSpecW <= 0 || allowedSpecH <= 0) {
        errorMsg = tr("图像边距过大 有效绘图区域不足");
        return false;
    }
    bool resizeNeeded = false;
    int finalW = imageToProcess.width();
    int finalH = imageToProcess.height();
    if (finalW > allowedSpecW) { finalW = allowedSpecW; resizeNeeded = true; }
    if (finalH > allowedSpecH) { finalH = allowedSpecH; resizeNeeded = true; }
    if (resizeNeeded) {
        imageToProcess = imageToProcess.scaled(finalW, finalH, Qt::IgnoreAspectRatio, Qt::FastTransformation);
        outIsResized = true;
    }
    QImage finalImage = drawComposedImage(imageToProcess, fileName, audioInfo.durationSec, audioInfo.sampleRate, settings);
    
    if (finalImage.isNull()) {
        errorMsg = tr("图像渲染失败");
        return false;
    }
    auto encoder = ImageEncoderFactory::createEncoder(settings.exportFormat);
    if (!encoder) {
        errorMsg = tr("无法创建编码器：") + settings.exportFormat;
        return false;
    }
    QByteArray encodedBytes = encoder->encodeToMemory(finalImage, settings.qualityLevel);
    qint64 tRender = perfTimer.elapsed();
    if (encodedBytes.isEmpty()) {
        errorMsg = tr("图像编码失败 可能是内存不足");
        return false;
    }
    outMetrics.filePath = filePath;
    outMetrics.decodingTimeMs = tDecode;
    outMetrics.fftTimeMs = tFft;
    outMetrics.renderingTimeMs = tRender;
    emit fileMetricsReported(outMetrics);
    QString outPath = generateOutputFilePath(filePath, settings);
    if (m_directWriteMode) {
        QFile file(outPath);
        if (!file.open(QIODevice::WriteOnly)) {
            errorMsg = tr("无法打开目标文件写入：") + outPath;
            return false;
        }
        if (file.write(encodedBytes) != encodedBytes.size()) {
            errorMsg = tr("写入不完整 可能是磁盘已满：") + outPath;
            return false;
        }
        file.close();
    } 
    else {
        BatWriteTask writeTask;
        writeTask.outputPath = outPath;
        writeTask.encodedData = std::move(encodedBytes);
        writeTask.width = finalImage.width();
        writeTask.height = finalImage.height();
        if (m_processor) {
            m_processor->enqueueWriteTask(std::move(writeTask));
        } else {
            errorMsg = tr("内部错误 指针无效");
            return false;
        }
    }
    return true;
}

QString BatWorker::generateOutputFilePath(const QString& inputFile, const BatSettings& settings) {
    QFileInfo inputFileInfo(inputFile);
    QString baseName = inputFileInfo.fileName();
    QString suffix;
    const QString& fmt = settings.exportFormat;
    if (fmt == "PNG" || fmt == "QtPNG") suffix = "png";
    else if (fmt == "BMP") suffix = "bmp";
    else if (fmt == "TIFF") suffix = "tif";
    else if (fmt == "JPG") suffix = "jpg";
    else if (fmt == "WebP") suffix = "webp";
    else if (fmt == "AVIF") suffix = "avif";
    else if (fmt == "JPEG 2000") suffix = "jp2";
    else suffix = "png";
    QString finalFileName = baseName + "." + suffix;
    QDir outputDir(settings.outputPath);
    if (settings.reuseSubfolderStructure) {
        QDir inputDir(settings.inputPath);
        QString relative = inputDir.relativeFilePath(inputFileInfo.absolutePath());
        if (relative.startsWith("..")) relative = ""; 
        QString subPath = QDir(inputDir.dirName()).filePath(relative);
        outputDir.mkpath(subPath);
        outputDir.cd(subPath);
    }
    return outputDir.absoluteFilePath(finalFileName);
}

int BatWorker::getRequiredFftSize(int height) const {
    const auto& opts = AppConfig::FFT_SIZE_OPTIONS;
    for (int size : opts) if (height <= (size / 2 + 1)) return size;
    return opts.back();
}
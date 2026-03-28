#include "BatchStreamWorker.h"
#include "BatchStreamDecoder.h"
#include "BatchStreamFft.h"
#include "BatchStreamGenerator.h"
#include "BatchStreamPainter.h"
#include "BatchStreamUtils.h"
#include "ImageEncoderFactory.h"
#include "AppConfig.h" 
#include "BatConfig.h"

#include <QFileInfo>
#include <QDir>
#include <QPainter>
#include <QElapsedTimer>
#include <QThread>
#include <algorithm>
#include <cmath>
#include <iostream>
#include <limits>
#include <cstring>

constexpr size_t READ_CHUNK_SIZE = 32768; 

BatchStreamWorker::BatchStreamWorker(int bucketId, int bucketIdPadding, std::atomic<bool>* stopFlag, std::atomic<bool>* pauseFlag, BatchStreamProcessor* processor, QObject* parent)
    : QObject(parent), m_bucketId(bucketId), m_bucketIdPadding(bucketIdPadding), m_stopFlag(stopFlag), m_pauseFlag(pauseFlag), m_processor(processor)
{}

void BatchStreamWorker::startWorkLoop(const BatchStreamSettings& settings) {
    QElapsedTimer bucketTimer;
    bucketTimer.start();
    BatchStreamFft fft;
    BatchStreamGenerator generator;
    while (true) {
        checkPause();
        if (m_stopFlag->load()) break;
        if (!m_processor) break;
        auto taskOpt = m_processor->claimTask(m_bucketId);
        if (!taskOpt.has_value()) {
            break;
        }
        BatchStreamTask task = taskOpt.value();
        QString filePath = task.fileInfo.path;
        QString fileName = QFileInfo(filePath).fileName();
        emit logMessage(QString(tr("%1 [线程 %2] 开始：%3\n"))
            .arg(BatchStreamUtils::getCurrentTimestamp())
            .arg(m_bucketId, m_bucketIdPadding, 10, QChar('0'))
            .arg(fileName));
        QString errorMsg;
        QElapsedTimer fileTimer;
        fileTimer.start();
        bool success = false;
        bool isResized = false;
        try {
            success = processSingleFile(task, settings, fft, generator, errorMsg, isResized);
        } catch (const std::exception& e) {
            errorMsg = QString(tr("发生异常: %1")).arg(e.what());
            success = false;
        } catch (...) {
            errorMsg = tr("发生未知错误");
            success = false;
        }
        if (task.buffer) {
            task.buffer = nullptr;
        }
        qint64 elapsed = fileTimer.elapsed();
        QString successMsg = isResized ? "WARNING_RESIZED" : "";
        emit fileFinished(filePath, success, success ? successMsg : errorMsg, m_bucketId, elapsed);
    }
    emit bucketFinished(m_bucketId, bucketTimer.elapsed());
}

void BatchStreamWorker::checkPause() {
    while (m_pauseFlag->load() && !m_stopFlag->load()) {
        QThread::msleep(100);
    }
}

int BatchStreamWorker::getRequiredFftSize(int height) const {
    const auto& opts = AppConfig::FFT_SIZE_OPTIONS;
    for (int size : opts) if (height <= (size / 2 + 1)) return size;
    return opts.back();
}

bool BatchStreamWorker::processSingleFile(BatchStreamTask& task, const BatchStreamSettings& settings, 
                                          BatchStreamFft& fft, BatchStreamGenerator& generator, QString& errorMsg, 
                                          bool& outIsResized) 
{
    outIsResized = false;
    QString filePath = task.fileInfo.path;
    BatchStreamDecoder decoder;
    BatchStreamAudioInfo info;
    bool openSuccess = false;
    if (task.buffer) {
        openSuccess = decoder.open(task.buffer, filePath, info);
    } else {
        openSuccess = decoder.open(filePath, info);
    }
    if (!openSuccess) {
        errorMsg = tr("无法打开音频流 解码器初始化失败");
        return false;
    }
    if (info.sampleRate <= 0) {
        errorMsg = tr("无效的音频流 采样率为0");
        return false;
    }
    if (info.durationSec <= 0.001) {
        if (task.fileInfo.duration > 0) {
            info.durationSec = static_cast<double>(task.fileInfo.duration) / 1000.0;
        } 
        else {
            info.durationSec = 300.0; 
        }
    }
    info.totalSamplesEst = static_cast<long long>(info.durationSec * info.sampleRate);
    int fftSize = getRequiredFftSize(settings.imageHeight);
    double effectiveInterval = settings.timeInterval;
    if (effectiveInterval <= 0.000000001) {
        effectiveInterval = static_cast<double>(fftSize) / info.sampleRate;
    }
    int hopSize = std::max(1, static_cast<int>(info.sampleRate * effectiveInterval));
    double overlap = 1.0 - (static_cast<double>(hopSize) / fftSize);
    bool useDouble = (info.sourceBitDepth > 32); 
    if (!fft.init(fftSize, overlap, settings.windowType, useDouble)) {
        errorMsg = tr("傅里叶变换初始化失败");
        return false;
    }
    generator.init(settings.imageHeight, fftSize, info.sampleRate, settings.curveType, settings.paletteType, settings.minDb, settings.maxDb);
    long long estWidthLong = (info.totalSamplesEst - fftSize) / hopSize + 1;
    if (estWidthLong <= 0) estWidthLong = 100;
    int estimatedWidth = static_cast<int>(std::min(estWidthLong, 500000LL)); 
    QImage rawSpectrogram(estimatedWidth, settings.imageHeight, QImage::Format_Indexed8);
    if (rawSpectrogram.isNull()) {
        errorMsg = QString(tr("内存不足 无法创建分辨率为 %1x%2 的图像")).arg(estimatedWidth).arg(settings.imageHeight);
        return false;
    }
    rawSpectrogram.fill(0); 
    int currentX = 0;
    long long totalSamplesProcessed = 0;
    while (!m_stopFlag->load()) {
        auto pcmData = decoder.readNextChunk(READ_CHUNK_SIZE);
        size_t currentChunkSize = std::visit([](auto&& arg){ return arg.size(); }, pcmData);
        if (currentChunkSize == 0) break;
        totalSamplesProcessed += currentChunkSize;
        auto spectrumData = fft.process(pcmData);
        QImage strip = generator.generateStrip(spectrumData);
        if (!strip.isNull() && strip.width() > 0) {
            if (rawSpectrogram.colorTable().isEmpty()) {
                rawSpectrogram.setColorTable(strip.colorTable());
            }
            int stripW = strip.width();
            int stripH = strip.height();
            if (currentX + stripW > rawSpectrogram.width()) {
                int growBy = std::max(rawSpectrogram.width() / 2, stripW + 1000);
                growBy = std::min(growBy, 50000);
                int newW = rawSpectrogram.width() + growBy;
                QImage newImg(newW, stripH, QImage::Format_Indexed8);
                if (newImg.isNull()) {
                    errorMsg = tr("内存不足 无法扩展图像 动态扩容失败");
                    break; 
                }
                newImg.setColorTable(rawSpectrogram.colorTable());
                newImg.fill(0);
                for(int y=0; y<stripH; ++y) {
                    std::memcpy(newImg.scanLine(y), rawSpectrogram.constScanLine(y), currentX);
                }
                rawSpectrogram = newImg;
            }
            for(int y=0; y<stripH; ++y) {
                std::memcpy(rawSpectrogram.scanLine(y) + currentX, strip.constScanLine(y), stripW);
            }
            currentX += stripW;
        }
    }
    if (!m_stopFlag->load()) {
        auto residualSpectrum = fft.flush();
        QImage residualStrip = generator.generateStrip(residualSpectrum);
        if (!residualStrip.isNull() && residualStrip.width() > 0) {
            int stripW = residualStrip.width();
            int stripH = residualStrip.height();
            if (currentX + stripW > rawSpectrogram.width()) {
                int newW = rawSpectrogram.width() + std::max(stripW, 100);
                QImage newImg(newW, stripH, QImage::Format_Indexed8);
                if (!newImg.isNull()) {
                    newImg.setColorTable(rawSpectrogram.colorTable());
                    newImg.fill(0);
                    for(int y=0; y<stripH; ++y) {
                        std::memcpy(newImg.scanLine(y), rawSpectrogram.constScanLine(y), currentX);
                    }
                    rawSpectrogram = newImg;
                }
            }
            if (currentX + stripW <= rawSpectrogram.width()) {
                for(int y=0; y<stripH; ++y) {
                    std::memcpy(rawSpectrogram.scanLine(y) + currentX, residualStrip.constScanLine(y), stripW);
                }
                currentX += stripW;
            }
        }
    }
    if (m_stopFlag->load()) {
        errorMsg = tr("任务已终止");
        return false;
    }
    if (currentX == 0) {
        errorMsg = tr("生成的图像为空 可能是全静音或解码数据异常");
        return false;
    }
    std::cout << "[Step 6] Finalizing..." << std::endl;
    QImage validSpectrogram = rawSpectrogram.copy(0, 0, currentX, settings.imageHeight);
    double pcmDuration = 0.0;
    if (info.sampleRate > 0) {
        pcmDuration = static_cast<double>(totalSamplesProcessed) / info.sampleRate;
    }
    if (pcmDuration > 0) {
        if (info.durationSec <= 0.001 || std::abs(info.durationSec - pcmDuration) > 1.0) {
             info.durationSec = pcmDuration;
        } 
        else {
             info.durationSec = pcmDuration;
        }
    }
    if (settings.enableWidthLimit && validSpectrogram.width() > settings.maxWidth) {
        std::cout << "  -> Resizing (User Limit) from " << validSpectrogram.width() << " to " << settings.maxWidth << std::endl;
        validSpectrogram = validSpectrogram.scaled(settings.maxWidth, settings.imageHeight, Qt::IgnoreAspectRatio, Qt::FastTransformation);
    }
    int maxFormatDim = std::numeric_limits<int>::max();
    if (settings.exportFormat == "JPG") {
        maxFormatDim = BatConfig::JPEG_MAX_DIMENSION;
    } else if (settings.exportFormat == "WebP") {
        maxFormatDim = BatConfig::BAT_WEBP_MAX_DIMENSION;
    } else if (settings.exportFormat == "AVIF") {
        maxFormatDim = BatConfig::BAT_AVIF_MAX_DIMENSION;
    }
    int topMargin = 0; int bottomMargin = 0; int leftMargin = 0; int rightMargin = 0;
    if (settings.enableComponents) {
        topMargin = AppConfig::PNG_SAVE_TOP_MARGIN;
        bottomMargin = AppConfig::PNG_SAVE_BOTTOM_MARGIN;
        leftMargin = AppConfig::PNG_SAVE_LEFT_MARGIN;
        rightMargin = AppConfig::PNG_SAVE_RIGHT_MARGIN;
    }
    int allowedSpecW = maxFormatDim - (leftMargin + rightMargin);
    int allowedSpecH = maxFormatDim - (topMargin + bottomMargin);
    if (allowedSpecW <= 0 || allowedSpecH <= 0) {
        errorMsg = tr("图像边距设置过大 导致有效绘图区域不足");
        return false;
    }
    bool resizeNeeded = false;
    int finalW = validSpectrogram.width();
    int finalH = validSpectrogram.height();
    if (finalW > allowedSpecW) { finalW = allowedSpecW; resizeNeeded = true; }
    if (finalH > allowedSpecH) { finalH = allowedSpecH; resizeNeeded = true; }
    if (resizeNeeded) {
        std::cout << "  -> Resizing (Format Limit) to " << finalW << "x" << finalH << std::endl;
        validSpectrogram = validSpectrogram.scaled(finalW, finalH, Qt::IgnoreAspectRatio, Qt::FastTransformation);
        outIsResized = true;
    }
    QImage finalImage = BatchStreamPainter::drawFinalImage(validSpectrogram, info, settings);
    if (finalImage.isNull()) {
        errorMsg = tr("图像绘制失败");
        return false;
    }
    std::cout << "[Step 7] Saving..." << std::endl;
    QString outPath = generateOutputFilePath(filePath, settings);
    auto encoder = ImageEncoderFactory::createEncoder(settings.exportFormat);
    if (!encoder) {
        errorMsg = tr("无法创建图像编码器: ") + settings.exportFormat;
        return false;
    }
    if (m_directWriteMode) {
        if (!encoder->encodeAndSave(finalImage, outPath, settings.qualityLevel)) {
            errorMsg = tr("保存文件失败: ") + outPath;
            return false;
        }
    } else {
        QByteArray encodedBytes = encoder->encodeToMemory(finalImage, settings.qualityLevel);
        if (encodedBytes.isEmpty()) {
            errorMsg = tr("图像编码失败 可能是内存不足");
            return false;
        }
        BatchStreamWriteTask writeTask;
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
    std::cout << "[Step 8] Done." << std::endl;
    return true;
}

QString BatchStreamWorker::generateOutputFilePath(const QString& inputFile, const BatchStreamSettings& settings) {
    QFileInfo inputFileInfo(inputFile);
    QString baseName = inputFileInfo.fileName();
    QString suffix;
    if (settings.exportFormat == "PNG" || settings.exportFormat == "QtPNG") suffix = "png";
    else if (settings.exportFormat == "BMP") suffix = "bmp";
    else if (settings.exportFormat == "TIFF") suffix = "tif";
    else if (settings.exportFormat == "JPG") suffix = "jpg";
    else if (settings.exportFormat == "WebP") suffix = "webp";
    else if (settings.exportFormat == "AVIF") suffix = "avif";
    else if (settings.exportFormat == "JPEG 2000") suffix = "jp2";
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
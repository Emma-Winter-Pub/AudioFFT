#include "BatWorker.h"
#include "BatAudioDecoder.h"
#include "BatFftProcessor.h"
#include "BatSpectrogramGenerator.h"
#include "BatImageExporter.h"
#include "BatUtils.h"

#include <vector>
#include <QDir>
#include <QThread>
#include <QFileInfo>
#include <QElapsedTimer>


BatWorker::BatWorker(int bucketId, int bucketIdPadding, std::atomic<bool>* stopFlag, std::atomic<bool>* pauseFlag, QObject *parent)
    : QObject(parent),
      m_bucketId(bucketId),
      m_bucketIdPadding(bucketIdPadding),
      m_stopFlag(stopFlag),
      m_pauseFlag(pauseFlag)
{
}


void BatWorker::processBucket(const Bucket& filesToProcess, const BatSettings& settings) {

    m_bucketTimer.start();

    BatAudioDecoder decoder;
    BatFftProcessor fftProcessor;
    BatSpectrogramGenerator generator;
    BatImageExporter exporter;

    QElapsedTimer stageTimer;

    for (const auto& fileInfo : filesToProcess)
    {
        checkPauseState();
        if (m_stopFlag->load()) {
            break;
        }

        emit logMessage(QString(tr("    [Task Group %1] Processing: %2"))
            .arg(m_bucketId, m_bucketIdPadding, 10, QChar('0'))
            .arg(QFileInfo(fileInfo.path).fileName()));

        FilePerformanceMetrics metrics;
        metrics.filePath = fileInfo.path;

        stageTimer.start();
        auto decodedResult = decoder.decode(fileInfo.path);
        metrics.decodingTimeMs = stageTimer.elapsed();

        if (!decodedResult.has_value()) {
            emit fileCompleted(fileInfo.path, false, tr("Audio decoding failed"));
            continue;
        }

        stageTimer.start();
        const std::vector<int> fftOptions = { 256, 512, 1024, 2048, 4096, 8192, 16384, 32768, 65536 };
        int fftSize = fftOptions.back();
        for (int size : fftOptions) {
            if (settings.imageHeight <= (size / 2 + 1)) {
                fftSize = size;
                break;
            }
        }
        auto spectrogramData = fftProcessor.compute(
            decodedResult->pcmData,
            settings.timeInterval,
            fftSize,
            decodedResult->sampleRate
        );
        metrics.fftTimeMs = stageTimer.elapsed();
        
        if (spectrogramData.empty()) {
            emit fileCompleted(fileInfo.path, false, tr("FFT calculation failed (Audio duration might be shorter than the selected precision)"));
            continue;
        }

        stageTimer.start();
        
        QImage rawSpectrogram = generator.generate(spectrogramData, settings.imageHeight, settings.curveType);
        
        if (rawSpectrogram.isNull()) {
            emit fileCompleted(fileInfo.path, false, tr("Spectrogram generation failed"));
            continue;
        }
        
        QString outputFilePath = generateOutputFilePath(fileInfo.path, settings);
        bool success = exporter.exportImage(
            rawSpectrogram,
            outputFilePath,
            QFileInfo(fileInfo.path).fileName(),
            settings,
            decodedResult.value()
        );
        metrics.renderingTimeMs = stageTimer.elapsed();

        if (!success) {
            emit fileCompleted(fileInfo.path, false, tr("Image saving failed"));
            continue;
        }

        emit fileCompleted(fileInfo.path, true, "");
        emit fileMetricsReported(metrics);
    }

    qint64 elapsedMs = m_bucketTimer.elapsed();
    emit bucketFinished(m_bucketId, elapsedMs);
}


void BatWorker::checkPauseState()
{
    while (m_pauseFlag->load() && !m_stopFlag->load()) {
        QThread::msleep(100);
    }
}


QString BatWorker::generateOutputFilePath(const QString& inputFile, const BatSettings& settings)
{
    QFileInfo inputFileInfo(inputFile);
    QString baseName = inputFileInfo.fileName();
    
    QString suffix;
    const QString& identifier = settings.exportFormat;
    if (identifier == "PNG" || identifier == "QtPNG") {
        suffix = "png";
    } else if (identifier == "BMP") {
        suffix = "bmp";
    } else if (identifier == "TIFF") {
        suffix = "tif";
    } else if (identifier == "JPG") {
        suffix = "jpg";
    } else if (identifier == "JPEG 2000") {
        suffix = "jp2";
    } else if (identifier == "WebP") {
        suffix = "webp";
    } else if (identifier == "AVIF") {
        suffix = "avif";
    } else {
        suffix = "png";
    }
    
    QString finalFileName = baseName + "." + suffix;
    
    QDir outputDir(settings.outputPath);

    if (settings.reuseSubfolderStructure) {
        QDir inputDir(settings.inputPath);
        
        QString inputRootName = inputDir.dirName();

        QString subfolderPath = inputDir.relativeFilePath(inputFileInfo.absolutePath());
        
        QString finalSubPath  = QDir(inputRootName).filePath(subfolderPath);

        outputDir.mkpath(finalSubPath);
        outputDir.cd(finalSubPath);
    }

    return outputDir.absoluteFilePath(finalFileName);
}
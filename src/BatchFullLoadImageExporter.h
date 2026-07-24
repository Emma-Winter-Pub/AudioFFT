#pragma once

#include <QImage>
#include <QString>

struct BatchSettings;
struct BatchFullLoadDecodedAudio;

enum class ExportStatus {
    Success,
    SuccessWithResize,
    Failure
};

class BatchFullLoadImageExporter {
public:
    BatchFullLoadImageExporter();
    ~BatchFullLoadImageExporter();
    ExportStatus exportImage(
        const QImage& rawSpectrogram,
        const QString& outputFilePath,
        const QString& sourceFileName,
        const BatchSettings& settings,
        const BatchFullLoadDecodedAudio& audioInfo,
        double minDb,
        double maxDb
    );

private:
    int calculateBestFreqStep(double maxFreqKhz, int availableHeight) const;
    int calculateBestDbStep(double dbRange, int availableHeight) const;
};
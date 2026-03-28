#pragma once

#include <QImage>
#include <QString>

struct BatSettings;
struct BatDecodedAudio;

enum class ExportStatus {
    Success,
    SuccessWithResize,
    Failure
};

class BatImageExporter {
public:
    BatImageExporter();
    ~BatImageExporter();
    ExportStatus exportImage(
        const QImage& rawSpectrogram,
        const QString& outputFilePath,
        const QString& sourceFileName,
        const BatSettings& settings,
        const BatDecodedAudio& audioInfo,
        double minDb,
        double maxDb
    );

private:
    int calculateBestFreqStep(double maxFreqKhz, int availableHeight) const;
    int calculateBestDbStep(double dbRange, int availableHeight) const;
};
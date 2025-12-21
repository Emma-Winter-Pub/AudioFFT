#pragma once

#include <QImage>
#include <QString>


struct BatSettings;


struct BatDecodedAudio;


class BatImageExporter{

public:
    BatImageExporter();
    ~BatImageExporter();

    bool exportImage(
        const QImage& rawSpectrogram,
        const QString& outputFilePath,
        const QString& sourceFileName,
        const BatSettings& settings,
        const BatDecodedAudio& audioInfo);

private:
    int calculateBestFreqStep(double maxFreqKhz, int availableHeight) const;
    int calculateBestDbStep(double dbRange, int availableHeight) const;

};
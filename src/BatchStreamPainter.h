#pragma once

#include "BatchStreamTypes.h"

#include <QImage>
#include <QString>

class BatchStreamPainter {
public:
    static QImage drawFinalImage(
        const QImage& rawSpectrogram,
        const BatchStreamAudioInfo& info,
        const BatchStreamSettings& settings
    );

private:
    static int calculateBestFreqStep(double maxFreqKhz, int availableHeight);
    static int calculateBestDbStep(double dbRange, int availableHeight);
};
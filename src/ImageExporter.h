#pragma once

#include "MappingCurves.h"

#include <QObject>
#include <QImage>


class ImageExporter {

public:
    struct ExportResult {
        bool success;
        QString message;
        QString outputFilePath;};

    static ExportResult exportImage(
        const QImage &spectrogramImage,
        const QString &fileName,
        double audioDuration,
        bool showGrid,
        const QString &preciseDurationStr,
        int nativeSampleRate,
        int quality,
        const QString &outputFilePath,
        const QString &formatIdentifier,
        CurveType curveType);
};
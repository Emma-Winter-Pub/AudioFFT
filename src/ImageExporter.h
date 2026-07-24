#pragma once

#include "MappingCurves.h"
#include "ColorPaletteFactory.h"

#include <QObject>
#include <QImage>
#include <QCoreApplication>

class ImageExporter
{
    Q_DECLARE_TR_FUNCTIONS(ImageExporter)

public:
    struct ExportResult {
        bool success;
        QString message;
        QString outputFilePath;
    };
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
        CurveType curveType,
        double minDb,
        double maxDb,
        const QString& paletteId,
        bool paletteInverted,
        bool paletteNegative,
        bool drawComponents = true
    );
};
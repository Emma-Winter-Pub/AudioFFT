#pragma once

#include "MappingCurves.h"

#include <vector>
#include <QList>
#include <QString>


extern "C" {
#include <libavcodec/avcodec.h>
}


struct FilePerformanceMetrics {
    QString filePath;
    qint64 decodingTimeMs = 0;
    qint64 fftTimeMs = 0;
    qint64 renderingTimeMs = 0;
};


struct FileInfo {
    QString path;
    qint64 duration;
    QString codecName;
    AVCodecID codecId;
};


using Bucket = QList<FileInfo>;


struct BatSettings {
    QString inputPath;
    QString outputPath;
    bool includeSubfolders;
    bool reuseSubfolderStructure;
    bool enableGrid;
    bool enableWidthLimit;
    int maxWidth;
    int imageHeight;
    double timeInterval;
    QString exportFormat;
    int qualityLevel;
    CurveType curveType = CurveType::Function0;
};
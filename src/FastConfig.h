#pragma once

#include <QColor>
#include <QFont>
#include <vector>

namespace FastConfig {
    constexpr int DEFAULT_WINDOW_WIDTH = 790;
    constexpr int DEFAULT_WINDOW_HEIGHT = 400;
    constexpr int MIN_WINDOW_WIDTH = 200;
    constexpr int MIN_WINDOW_HEIGHT = 100;
    constexpr int LOG_AREA_DEFAULT_WIDTH = 550;
    constexpr int LOG_AREA_DEFAULT_HEIGHT = 300;
    constexpr int DEFAULT_SPECTROGRAM_HEIGHT = 1025;
    constexpr int DEFAULT_FFT_SIZE = 4096;
    const std::vector<int> FFT_SIZE_OPTIONS = { 128, 256, 512, 1024, 2048, 4096, 8192, 16384, 32768 };
    constexpr int VIEWER_TOP_MARGIN = 20;
    constexpr int VIEWER_BOTTOM_MARGIN = 20;
    constexpr int VIEWER_LEFT_MARGIN = 30;
    constexpr int VIEWER_RIGHT_MARGIN = 50;
    constexpr int VIEWER_COLOR_BAR_WIDTH = 15;
    constexpr int PNG_SAVE_TOP_MARGIN = 30;
    constexpr int PNG_SAVE_BOTTOM_MARGIN = 30;
    constexpr int PNG_SAVE_LEFT_MARGIN = 35;
    constexpr int PNG_SAVE_RIGHT_MARGIN = 55;
    constexpr int PNG_SAVE_COLOR_BAR_WIDTH = 15;
    constexpr int PNG_SAVE_DEFAULT_MAX_WIDTH = 2000;
    constexpr int PNG_SAVE_MIN_MAX_WIDTH = 500;
    constexpr int PNG_SAVE_MAX_MAX_WIDTH = 10000;
    constexpr int PNG_SAVE_DEFAULT_COMPRESSION_LEVEL = 6;
    constexpr int JPG_SAVE_DEFAULT_QUALITY = 85;
    constexpr int JPEG_MAX_DIMENSION = 65535;
    constexpr int MY_WEBP_MAX_DIMENSION = 16383;
    constexpr int MY_AVIF_MAX_DIMENSION = 8192;
    constexpr double DEFAULT_MIN_DB = -130.0;
    constexpr double DEFAULT_MAX_DB = 0.0;
    const QFont TEXT_FONT("", 12);
    const QColor TEXT_COLOR(Qt::white);
    const QColor PNG_SAVE_GRID_LINE_COLOR(Qt::white); 
    const QColor VIEWER_GRID_LINE_COLOR(240, 240, 240, 240); 
    constexpr int GRID_LINE_WIDTH = 1;
    constexpr double DEFAULT_TIME_INTERVAL_S = 0.0;  // Auto = 0.0
    constexpr double MIN_TIME_INTERVAL_S = 0.01;
    constexpr double MAX_TIME_INTERVAL_S = 1.0;
}
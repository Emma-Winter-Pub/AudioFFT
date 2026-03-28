#pragma once

#include "MappingCurves.h" 
#include "ColorPalette.h"
#include "WindowFunctions.h"

#include <QColor>
#include <QFont>
#include <vector>
#include <QStringList>
#include <cmath>
#include <algorithm>
#include <string>
#include <map>
#include <sstream>

namespace BatConfig {
    constexpr int JPEG_MAX_DIMENSION = 65500;
    constexpr int BAT_WEBP_MAX_DIMENSION = 16383;
    constexpr int BAT_AVIF_MAX_DIMENSION = 8192;
    constexpr int FINAL_IMAGE_TOP_MARGIN = 30;
    constexpr int FINAL_IMAGE_BOTTOM_MARGIN = 30;
    constexpr int FINAL_IMAGE_LEFT_MARGIN = 35;
    constexpr int FINAL_IMAGE_RIGHT_MARGIN = 55;
    constexpr bool DEFAULT_INCLUDE_SUBFOLDERS = true;
    constexpr bool DEFAULT_REUSE_STRUCTURE = true;
    constexpr int DEFAULT_IMAGE_HEIGHT = 513;
    constexpr double DEFAULT_TIME_INTERVAL = 0.0;  // Auto = 0.0
    constexpr WindowType DEFAULT_WINDOW_TYPE = WindowType::Hann;
    constexpr CurveType DEFAULT_CURVE_TYPE = CurveType::XX; 
    constexpr PaletteType DEFAULT_PALETTE_TYPE = PaletteType::S01;
    constexpr double DEFAULT_MAX_DB = 0.0;
    constexpr double DEFAULT_MIN_DB = -130.0;
    constexpr bool DEFAULT_ENABLE_GRID = false;
    constexpr bool DEFAULT_ENABLE_WIDTH_LIMIT = true;
    constexpr int DEFAULT_MAX_WIDTH = 1000;
    constexpr char DEFAULT_EXPORT_FORMAT[] = "PNG";
    constexpr int DEFAULT_QUALITY_LEVEL = 6;
}
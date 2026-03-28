#pragma once

#include "WindowFunctions.h"
#include "MappingCurves.h"
#include "ColorPalette.h"
#include "AppConfig.h"

#include <QColor>
#include <QSettings>
#include <QString>
#include <QKeySequence>

enum class SpectrumProfileType { Line, Bar };
enum class DataSourceType { ImagePixel = 0, FftRawData = 1 };

struct GlobalPreferences {
    bool showLog = false;
    bool showGrid = false;
    bool showComponents = true;
    bool enableZoom = false;
    bool enableWidthLimit = false; 
    int maxWidth = 2000;
    bool showSpectrumProfile = true;
    QColor spectrumProfileColor = Qt::yellow;
    int spectrumProfileLineWidth = 1;
    bool spectrumProfileFilled = true;
    int spectrumProfileFillAlpha = 30;
    SpectrumProfileType spectrumProfileType = SpectrumProfileType::Line;
    bool enableCrosshair = true;
    int height = 1025;
    double timeInterval = 0.0; // 0.0 = Auto
    WindowType windowType = WindowType::Hann;
    CurveType curveType = CurveType::XX;
    PaletteType paletteType = PaletteType::S01;
    double minDb = -130.0;
    double maxDb = 0.0;
    int crosshairLength = 10000;
    int crosshairWidth = 1;
    QColor crosshairColor = QColor(255, 255, 255);
    bool showCoordFreq = true;
    bool showCoordTime = true;
    bool showCoordDb = true;
    bool cacheFftData = false;
    bool enableGpuAcceleration = false;
    DataSourceType spectrumSource = DataSourceType::ImagePixel;
    DataSourceType probeSource = DataSourceType::ImagePixel;
    int probeDbPrecision = 15;
    bool playheadVisible = true;
    int playheadLineWidth = 1;
    QColor playheadColor = QColor(45, 140, 235);
    QColor playheadHandleColor = QColor(45, 140, 235);
    int playerFrameRate = 60;
    int profileFrameRate = 60;
    QKeySequence screenshotHotkey1;
    QKeySequence screenshotHotkey2;
    QKeySequence quickCopyHotkey;
    bool hideMouseCursor = true;
    bool copyToClipboard = false;
    static GlobalPreferences load();
    static void save(const GlobalPreferences& prefs);
    void resetToDefaults();
};
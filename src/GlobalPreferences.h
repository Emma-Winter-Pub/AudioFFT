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
enum class SpectrumProfileDirection { Horizontal, Vertical };
enum class DataSourceType { ImagePixel = 0, FftRawData = 1 };

struct GlobalPreferences {

    // 常规
    bool allowMultipleInstances = true;
    bool autoPlayOnExternalOpen = true;
    int defaultWorkspace = 1;
    bool showLog = false;
    bool showGrid = false;
    bool showComponents = true;
    bool enableZoom = false;
    bool enableWidthLimit = false; 
    int maxWidth = 2000;

    // 性能
    bool enableGpuAcceleration = true;
    bool cacheFftData = true;
    int playerFrameRate = 60;
    int profileFrameRate = 60;

    // 频谱图
    int height = 1025;
    double timeInterval = 0.0; // 0.0 = Auto
    WindowType windowType = WindowType::Hann;
    CurveType curveType = CurveType::XX;
    PaletteType paletteType = PaletteType::S01;
    double maxDb = 0.0;
    double minDb = -130.0;

    // 频率分布
    bool showSpectrumProfile = true;
    DataSourceType spectrumSource = DataSourceType::FftRawData;
    SpectrumProfileType spectrumProfileType = SpectrumProfileType::Line;
    SpectrumProfileDirection spectrumProfileDirection = SpectrumProfileDirection::Horizontal;
    int spectrumProfileLineWidth = 1;
    QColor spectrumProfileColor = QColor(255, 255, 255);
    bool spectrumProfileFilled = true;
    int spectrumProfileFillAlpha = 35;

    // 十字光标
    bool enableCrosshair = true;
    int crosshairLength = 10000;
    int crosshairWidth = 1;
    QColor crosshairColor = QColor(255, 255, 255);

    // 探针
    bool showCoordFreq = true;
    bool showCoordTime = true;
    bool showCoordDb = true;
    DataSourceType probeSource = DataSourceType::FftRawData;
    int probeDbPrecision = 15;

    // 播放器
    bool playheadVisible = true;
    int playheadLineWidth = 1;
    QColor playheadHandleColor = QColor(45, 140, 235);
    QColor playheadColor = QColor(45, 140, 235);

    // 截图
    bool hideMouseCursor = false;
    bool copyToClipboard = false;
    QKeySequence screenshotHotkey1 = QKeySequence(Qt::Key_F2);
    QKeySequence screenshotHotkey2 = QKeySequence("Ctrl+P");
    QKeySequence quickCopyHotkey   = QKeySequence(Qt::Key_F12);

    static GlobalPreferences load();
    static void save(const GlobalPreferences& prefs);
    void resetToDefaults();
};
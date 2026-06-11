#include "GlobalPreferences.h"

#include <QCoreApplication>
#include <QDir>

void GlobalPreferences::resetToDefaults() {

    // 常规
    allowMultipleInstances = true;
    autoPlayOnExternalOpen = true;
    defaultWorkspace = 1;
    showLog = false;
    showGrid = false;
    showComponents = true;
    enableZoom = false;
    enableWidthLimit = false;
    maxWidth = 2000;

    // 性能
    enableGpuAcceleration = true;
    cacheFftData = true;
    playerFrameRate = 60;
    profileFrameRate = 60;

    // 频谱图
    height = 1025;
    timeInterval = 0.0;
    windowType = WindowType::Hann;
    curveType = CurveType::XX;
    paletteType = PaletteType::S01;
    maxDb = 0.0;
    minDb = -130.0;

    // 频率分布
    showSpectrumProfile = true;
    spectrumSource = DataSourceType::FftRawData;
    spectrumProfileType = SpectrumProfileType::Line;
    spectrumProfileDirection = SpectrumProfileDirection::Horizontal;
    spectrumProfileLineWidth = 1;
    spectrumProfileColor = QColor(255, 255, 255);
    spectrumProfileFilled = true;
    spectrumProfileFillAlpha = 35;

    // 十字光标
    enableCrosshair = true;
    crosshairLength = 10000;
    crosshairWidth = 1;
    crosshairColor = QColor(255, 255, 255);

    // 探针
    showCoordFreq = true;
    showCoordTime = true;
    showCoordDb = true;
    probeSource = DataSourceType::FftRawData;
    probeDbPrecision = 15;

    // 播放器
    playheadVisible = true;
    playheadLineWidth = 1;
    playheadHandleColor = QColor(45, 140, 235);
    playheadColor = QColor(45, 140, 235);

    // 截图
    hideMouseCursor = false;
    copyToClipboard = false;
    screenshotHotkey1 = QKeySequence(Qt::Key_F2);
    screenshotHotkey2 = QKeySequence("Ctrl+P");
    quickCopyHotkey   = QKeySequence(Qt::Key_F12);
}

GlobalPreferences GlobalPreferences::load() {
    GlobalPreferences prefs;
    prefs.resetToDefaults();
    QString configPath = QCoreApplication::applicationDirPath() + "/AudioFFT_Config.ini";
    QSettings settings(configPath, QSettings::IniFormat);
    settings.beginGroup("GlobalPreferences");
    if (settings.childKeys().isEmpty()) {
        settings.endGroup();
        return prefs;
    }

    // 常规
    prefs.allowMultipleInstances = settings.value("allowMultipleInstances", prefs.allowMultipleInstances).toBool();
    prefs.autoPlayOnExternalOpen = settings.value("autoPlayOnExternalOpen", prefs.autoPlayOnExternalOpen).toBool();
    prefs.defaultWorkspace = settings.value("defaultWorkspace", prefs.defaultWorkspace).toInt();
    prefs.showLog = settings.value("showLog", prefs.showLog).toBool();
    prefs.showGrid = settings.value("showGrid", prefs.showGrid).toBool();
    prefs.showComponents = settings.value("showComponents", prefs.showComponents).toBool();
    prefs.enableZoom = settings.value("enableZoom", prefs.enableZoom).toBool();
    prefs.enableWidthLimit = settings.value("enableWidthLimit", prefs.enableWidthLimit).toBool();
    prefs.maxWidth = settings.value("maxWidth", prefs.maxWidth).toInt();

    // 性能
    prefs.enableGpuAcceleration = settings.value("enableGpuAcceleration", prefs.enableGpuAcceleration).toBool();
    prefs.cacheFftData = settings.value("cacheFftData", prefs.cacheFftData).toBool();
    prefs.playerFrameRate = settings.value("playerFrameRate", prefs.playerFrameRate).toInt();
    prefs.profileFrameRate = settings.value("profileFrameRate", prefs.profileFrameRate).toInt();
    if (prefs.profileFrameRate >= prefs.playerFrameRate) {
        prefs.playerFrameRate = prefs.profileFrameRate;
    }

    // 频谱图
    prefs.height = settings.value("height", prefs.height).toInt();
    prefs.timeInterval = settings.value("timeInterval", prefs.timeInterval).toDouble();
    prefs.windowType = static_cast<WindowType>(settings.value("windowType", static_cast<int>(prefs.windowType)).toInt());
    prefs.curveType = static_cast<CurveType>(settings.value("curveType", static_cast<int>(prefs.curveType)).toInt());
    prefs.paletteType = static_cast<PaletteType>(settings.value("paletteType", static_cast<int>(prefs.paletteType)).toInt());
    prefs.maxDb = settings.value("maxDb", prefs.maxDb).toDouble();
    prefs.minDb = settings.value("minDb", prefs.minDb).toDouble();

    // 频率分布
    prefs.showSpectrumProfile = settings.value("showSpectrumProfile", prefs.showSpectrumProfile).toBool();
    prefs.spectrumSource = static_cast<DataSourceType>(settings.value("spectrumSource", static_cast<int>(prefs.spectrumSource)).toInt());
    prefs.spectrumProfileType = static_cast<SpectrumProfileType>(settings.value("spectrumProfileType", static_cast<int>(prefs.spectrumProfileType)).toInt());
    prefs.spectrumProfileDirection = static_cast<SpectrumProfileDirection>(settings.value("spectrumProfileDirection", static_cast<int>(prefs.spectrumProfileDirection)).toInt());
    prefs.spectrumProfileLineWidth = settings.value("spectrumProfileLineWidth", prefs.spectrumProfileLineWidth).toInt();
    QVariant profCol = settings.value("spectrumProfileColor");
    if (profCol.isValid() && !profCol.isNull()) prefs.spectrumProfileColor = profCol.value<QColor>();
    prefs.spectrumProfileFilled = settings.value("spectrumProfileFilled", prefs.spectrumProfileFilled).toBool();
    prefs.spectrumProfileFillAlpha = settings.value("spectrumProfileFillAlpha", prefs.spectrumProfileFillAlpha).toInt();

    // 十字光标
    prefs.enableCrosshair = settings.value("enableCrosshair", prefs.enableCrosshair).toBool();
    prefs.crosshairLength = settings.value("crosshairLength", prefs.crosshairLength).toInt();
    prefs.crosshairWidth = settings.value("crosshairWidth", prefs.crosshairWidth).toInt();
    QVariant crossCol = settings.value("crosshairColor");
    if (crossCol.isValid() && !crossCol.isNull()) prefs.crosshairColor = crossCol.value<QColor>();

    // 探针
    prefs.showCoordFreq = settings.value("showCoordFreq", prefs.showCoordFreq).toBool();
    prefs.showCoordTime = settings.value("showCoordTime", prefs.showCoordTime).toBool();
    prefs.showCoordDb   = settings.value("showCoordDb", prefs.showCoordDb).toBool();
    prefs.probeSource = static_cast<DataSourceType>(settings.value("probeSource", static_cast<int>(prefs.probeSource)).toInt());
    prefs.probeDbPrecision = settings.value("probeDbPrecision", prefs.probeDbPrecision).toInt();

    // 播放器
    prefs.playheadVisible = settings.value("playheadVisible", prefs.playheadVisible).toBool();
    prefs.playheadLineWidth = settings.value("playheadLineWidth", prefs.playheadLineWidth).toInt();
    QVariant phHCol = settings.value("playheadHandleColor");
    if (phHCol.isValid() && !phHCol.isNull()) prefs.playheadHandleColor = phHCol.value<QColor>();
    QVariant phCol = settings.value("playheadColor");
    if (phCol.isValid() && !phCol.isNull()) prefs.playheadColor = phCol.value<QColor>();

    // 截图
    prefs.hideMouseCursor = settings.value("hideMouseCursor", prefs.hideMouseCursor).toBool();
    prefs.copyToClipboard = settings.value("copyToClipboard", prefs.copyToClipboard).toBool();
    QString key1 = settings.value("screenshotHotkey1", prefs.screenshotHotkey1.toString(QKeySequence::PortableText)).toString();
    QString key2 = settings.value("screenshotHotkey2", prefs.screenshotHotkey2.toString(QKeySequence::PortableText)).toString();
    QString key3 = settings.value("quickCopyHotkey", prefs.quickCopyHotkey.toString(QKeySequence::PortableText)).toString();
    prefs.screenshotHotkey1 = QKeySequence(key1);
    prefs.screenshotHotkey2 = QKeySequence(key2);
    prefs.quickCopyHotkey   = QKeySequence(key3);

    settings.endGroup();
    return prefs;
}

void GlobalPreferences::save(const GlobalPreferences& prefs) {
    QString configPath = QCoreApplication::applicationDirPath() + "/AudioFFT_Config.ini";
    QSettings settings(configPath, QSettings::IniFormat);
    settings.beginGroup("GlobalPreferences");

    // 常规
    settings.setValue("allowMultipleInstances", prefs.allowMultipleInstances);
    settings.setValue("autoPlayOnExternalOpen", prefs.autoPlayOnExternalOpen);
    settings.setValue("defaultWorkspace", prefs.defaultWorkspace);
    settings.setValue("showLog", prefs.showLog);
    settings.setValue("showGrid", prefs.showGrid);
    settings.setValue("showComponents", prefs.showComponents);
    settings.setValue("enableZoom", prefs.enableZoom);
    settings.setValue("enableWidthLimit", prefs.enableWidthLimit);
    settings.setValue("maxWidth", prefs.maxWidth);

    // 性能
    settings.setValue("enableGpuAcceleration", prefs.enableGpuAcceleration);
    settings.setValue("cacheFftData", prefs.cacheFftData);
    settings.setValue("playerFrameRate", prefs.playerFrameRate);
    settings.setValue("profileFrameRate", prefs.profileFrameRate);

    // 频谱图
    settings.setValue("height", prefs.height);
    settings.setValue("timeInterval", prefs.timeInterval);
    settings.setValue("windowType", static_cast<int>(prefs.windowType));
    settings.setValue("curveType", static_cast<int>(prefs.curveType));
    settings.setValue("paletteType", static_cast<int>(prefs.paletteType));
    settings.setValue("maxDb", prefs.maxDb);
    settings.setValue("minDb", prefs.minDb);

    // 频率分布
    settings.setValue("showSpectrumProfile", prefs.showSpectrumProfile);
    settings.setValue("spectrumSource", static_cast<int>(prefs.spectrumSource));
    settings.setValue("spectrumProfileType", static_cast<int>(prefs.spectrumProfileType));
    settings.setValue("spectrumProfileDirection", static_cast<int>(prefs.spectrumProfileDirection));
    settings.setValue("spectrumProfileLineWidth", prefs.spectrumProfileLineWidth);
    settings.setValue("spectrumProfileColor", prefs.spectrumProfileColor);
    settings.setValue("spectrumProfileFilled", prefs.spectrumProfileFilled);
    settings.setValue("spectrumProfileFillAlpha", prefs.spectrumProfileFillAlpha);

    // 十字光标
    settings.setValue("enableCrosshair", prefs.enableCrosshair);
    settings.setValue("crosshairLength", prefs.crosshairLength);
    settings.setValue("crosshairWidth", prefs.crosshairWidth);
    settings.setValue("crosshairColor", prefs.crosshairColor);

    // 探针
    settings.setValue("showCoordFreq", prefs.showCoordFreq);
    settings.setValue("showCoordTime", prefs.showCoordTime);
    settings.setValue("showCoordDb", prefs.showCoordDb);
    settings.setValue("probeSource", static_cast<int>(prefs.probeSource));
    settings.setValue("probeDbPrecision", prefs.probeDbPrecision);

    // 播放器
    settings.setValue("playheadVisible", prefs.playheadVisible);
    settings.setValue("playheadLineWidth", prefs.playheadLineWidth);
    settings.setValue("playheadHandleColor", prefs.playheadHandleColor);
    settings.setValue("playheadColor", prefs.playheadColor);

    // 截图
    settings.setValue("hideMouseCursor", prefs.hideMouseCursor);
    settings.setValue("copyToClipboard", prefs.copyToClipboard);
    settings.setValue("screenshotHotkey1", prefs.screenshotHotkey1.toString(QKeySequence::PortableText));
    settings.setValue("screenshotHotkey2", prefs.screenshotHotkey2.toString(QKeySequence::PortableText));
    settings.setValue("quickCopyHotkey",   prefs.quickCopyHotkey.toString(QKeySequence::PortableText));

    settings.endGroup();
    settings.sync();
}
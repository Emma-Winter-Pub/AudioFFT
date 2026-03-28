#include "GlobalPreferences.h"

#include <QCoreApplication>
#include <QDir>

void GlobalPreferences::resetToDefaults() {
    showLog = false;
    showGrid = false;
    showComponents = true;
    enableZoom = false;
    enableWidthLimit = false;
    maxWidth = 2000;
    showSpectrumProfile = true;
    spectrumProfileColor = QColor(255, 255, 255);
    spectrumProfileLineWidth = 1;
    spectrumProfileFilled = true;
    spectrumProfileFillAlpha = 35;
    spectrumProfileType = SpectrumProfileType::Line;
    enableCrosshair = true;
    height = 1025;
    timeInterval = 0.0;
    windowType = WindowType::Hann;
    curveType = CurveType::XX;
    paletteType = PaletteType::S01;
    minDb = -130.0;
    maxDb = 0.0;
    crosshairLength = 10000;
    crosshairWidth = 1;
    crosshairColor = QColor(255, 255, 255);
    showCoordFreq = true;
    showCoordTime = true;
    showCoordDb = true;
    cacheFftData = true;
    enableGpuAcceleration = true;
    spectrumSource = DataSourceType::FftRawData;
    probeSource = DataSourceType::FftRawData;
    probeDbPrecision = 15;
    playheadVisible = true;
    playheadLineWidth = 1;
    playheadColor = QColor(45, 140, 235);
    playheadHandleColor = QColor(45, 140, 235);
    playerFrameRate = 60;
    profileFrameRate = 60;
    screenshotHotkey1 = QKeySequence(Qt::Key_F2);
    screenshotHotkey2 = QKeySequence("Ctrl+P");
    quickCopyHotkey   = QKeySequence(Qt::Key_F12);
    hideMouseCursor = false;
    copyToClipboard = false;
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
    prefs.showLog = settings.value("showLog", false).toBool();
    prefs.showGrid = settings.value("showGrid", false).toBool();
    prefs.showComponents = settings.value("showComponents", true).toBool();
    prefs.enableZoom = settings.value("enableZoom", false).toBool();
    prefs.enableWidthLimit = settings.value("enableWidthLimit", false).toBool();
    prefs.enableCrosshair = settings.value("enableCrosshair", true).toBool();
    prefs.maxWidth = settings.value("maxWidth", 2000).toInt();
    prefs.showSpectrumProfile = settings.value("showSpectrumProfile", true).toBool();
    prefs.spectrumProfileLineWidth = settings.value("spectrumProfileLineWidth", 1).toInt();
    QVariant profCol = settings.value("spectrumProfileColor");
    if (profCol.isValid() && !profCol.isNull()) prefs.spectrumProfileColor = profCol.value<QColor>();
    prefs.spectrumProfileFilled = settings.value("spectrumProfileFilled", true).toBool();
    prefs.spectrumProfileFillAlpha = settings.value("spectrumProfileFillAlpha", 30).toInt();
    prefs.spectrumProfileType = static_cast<SpectrumProfileType>(settings.value("spectrumProfileType", 0).toInt());
    prefs.height = settings.value("height", 1025).toInt();
    prefs.timeInterval = settings.value("timeInterval", 0.0).toDouble();
    prefs.minDb = settings.value("minDb", -130.0).toDouble();
    prefs.maxDb = settings.value("maxDb", 0.0).toDouble();
    prefs.windowType = static_cast<WindowType>(settings.value("windowType", static_cast<int>(WindowType::Hann)).toInt());
    prefs.curveType = static_cast<CurveType>(settings.value("curveType", static_cast<int>(CurveType::XX)).toInt());
    prefs.paletteType = static_cast<PaletteType>(settings.value("paletteType", static_cast<int>(PaletteType::S01)).toInt());
    prefs.crosshairLength = settings.value("crosshairLength", 10000).toInt();
    prefs.crosshairWidth = settings.value("crosshairWidth", 1).toInt();
    QVariant colVar = settings.value("crosshairColor");
    if (colVar.isValid() && !colVar.isNull()) {
        prefs.crosshairColor = colVar.value<QColor>();
    }
    prefs.showCoordFreq = settings.value("showCoordFreq", true).toBool();
    prefs.showCoordTime = settings.value("showCoordTime", true).toBool();
    prefs.showCoordDb   = settings.value("showCoordDb", true).toBool();
    prefs.cacheFftData = settings.value("cacheFftData", false).toBool();
    prefs.enableGpuAcceleration = settings.value("enableGpuAcceleration", false).toBool();
    prefs.spectrumSource = static_cast<DataSourceType>(settings.value("spectrumSource", 0).toInt());
    prefs.probeSource = static_cast<DataSourceType>(settings.value("probeSource", 0).toInt());
    prefs.probeDbPrecision = settings.value("probeDbPrecision", 1).toInt();
    prefs.playheadVisible = settings.value("playheadVisible", true).toBool();
    prefs.playheadLineWidth = settings.value("playheadLineWidth", 1).toInt();
    QVariant phCol = settings.value("playheadColor");
    if (phCol.isValid() && !phCol.isNull()) prefs.playheadColor = phCol.value<QColor>();
    QVariant phHCol = settings.value("playheadHandleColor");
    if (phHCol.isValid() && !phHCol.isNull()) prefs.playheadHandleColor = phHCol.value<QColor>();
    prefs.playerFrameRate = settings.value("playerFrameRate", 60).toInt();
    prefs.profileFrameRate = settings.value("profileFrameRate", 30).toInt();
    QString key1 = settings.value("screenshotHotkey1", "F2").toString();
    QString key2 = settings.value("screenshotHotkey2", "Ctrl+P").toString();
    QString key3 = settings.value("quickCopyHotkey", "F12").toString();
    prefs.screenshotHotkey1 = QKeySequence(key1);
    prefs.screenshotHotkey2 = QKeySequence(key2);
    prefs.quickCopyHotkey   = QKeySequence(key3);
    prefs.hideMouseCursor = settings.value("hideMouseCursor", true).toBool();
    prefs.copyToClipboard = settings.value("copyToClipboard", false).toBool();

    settings.endGroup();
    return prefs;
}

void GlobalPreferences::save(const GlobalPreferences& prefs) {
    QString configPath = QCoreApplication::applicationDirPath() + "/AudioFFT_Config.ini";
    QSettings settings(configPath, QSettings::IniFormat);
    settings.beginGroup("GlobalPreferences");
    settings.setValue("showLog", prefs.showLog);
    settings.setValue("showGrid", prefs.showGrid);
    settings.setValue("showComponents", prefs.showComponents);
    settings.setValue("enableZoom", prefs.enableZoom);
    settings.setValue("enableWidthLimit", prefs.enableWidthLimit);
    settings.setValue("enableCrosshair", prefs.enableCrosshair);
    settings.setValue("maxWidth", prefs.maxWidth);
    settings.setValue("showSpectrumProfile", prefs.showSpectrumProfile);
    settings.setValue("spectrumProfileLineWidth", prefs.spectrumProfileLineWidth);
    settings.setValue("spectrumProfileColor", prefs.spectrumProfileColor);
    settings.setValue("spectrumProfileFilled", prefs.spectrumProfileFilled);
    settings.setValue("spectrumProfileFillAlpha", prefs.spectrumProfileFillAlpha);
    settings.setValue("spectrumProfileType", static_cast<int>(prefs.spectrumProfileType));
    settings.setValue("height", prefs.height);
    settings.setValue("timeInterval", prefs.timeInterval);
    settings.setValue("minDb", prefs.minDb);
    settings.setValue("maxDb", prefs.maxDb);
    settings.setValue("windowType", static_cast<int>(prefs.windowType));
    settings.setValue("curveType", static_cast<int>(prefs.curveType));
    settings.setValue("paletteType", static_cast<int>(prefs.paletteType));
    settings.setValue("crosshairLength", prefs.crosshairLength);
    settings.setValue("crosshairWidth", prefs.crosshairWidth);
    settings.setValue("crosshairColor", prefs.crosshairColor);
    settings.setValue("showCoordFreq", prefs.showCoordFreq);
    settings.setValue("showCoordTime", prefs.showCoordTime);
    settings.setValue("showCoordDb",   prefs.showCoordDb);
    settings.setValue("cacheFftData", prefs.cacheFftData);
    settings.setValue("enableGpuAcceleration", prefs.enableGpuAcceleration);
    settings.setValue("spectrumSource", static_cast<int>(prefs.spectrumSource));
    settings.setValue("probeSource", static_cast<int>(prefs.probeSource));
    settings.setValue("probeDbPrecision", prefs.probeDbPrecision);
    settings.setValue("playheadVisible", prefs.playheadVisible);
    settings.setValue("playheadLineWidth", prefs.playheadLineWidth);
    settings.setValue("playheadColor", prefs.playheadColor);
    settings.setValue("playheadHandleColor", prefs.playheadHandleColor);
    settings.setValue("playerFrameRate", prefs.playerFrameRate);
    settings.setValue("profileFrameRate", prefs.profileFrameRate);
    settings.setValue("screenshotHotkey1", prefs.screenshotHotkey1.toString(QKeySequence::PortableText));
    settings.setValue("screenshotHotkey2", prefs.screenshotHotkey2.toString(QKeySequence::PortableText));
    settings.setValue("quickCopyHotkey",   prefs.quickCopyHotkey.toString(QKeySequence::PortableText));
    settings.setValue("hideMouseCursor", prefs.hideMouseCursor);
    settings.setValue("copyToClipboard", prefs.copyToClipboard);
    settings.endGroup();
    settings.sync();
}
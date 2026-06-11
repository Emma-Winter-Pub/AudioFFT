#include "CrosshairConfig.h"

#include <QSettings>
#include <QCoreApplication>
#include <QDir>
#include <QVariant>

CrosshairStyle CrosshairConfig::load() {
    CrosshairStyle style; 
    QString configPath = QCoreApplication::applicationDirPath() + "/AudioFFT_Config.ini";
    QSettings settings(configPath, QSettings::IniFormat);
    settings.beginGroup("Crosshair");
    if (settings.childKeys().isEmpty()) {
        settings.endGroup();
        return style;
    }
    style.lineLength = settings.value("LineLength", 10000).toInt();
    style.lineWidth = settings.value("LineWidth", 1).toInt();
    QVariant colVar = settings.value("Color");
    if (colVar.isValid() && !colVar.isNull()) {
        style.color = colVar.value<QColor>();
    }
    settings.endGroup();
    if (style.lineLength < 100) style.lineLength = 100;
    if (style.lineLength > 10000) style.lineLength = 10000;
    if (style.lineWidth < 1) style.lineWidth = 1;
    if (style.lineWidth > 15) style.lineWidth = 15;
    if (!style.color.isValid()) {
        style.color = Qt::white;
    }
    return style;
}

void CrosshairConfig::save(const CrosshairStyle& style) {
    QString configPath = QCoreApplication::applicationDirPath() + "/AudioFFT_Config.ini";
    QSettings settings(configPath, QSettings::IniFormat);
    settings.beginGroup("Crosshair");
    settings.setValue("LineLength", style.lineLength);
    settings.setValue("LineWidth", style.lineWidth);
    settings.setValue("Color", style.color);
    settings.endGroup();
    settings.sync();
}
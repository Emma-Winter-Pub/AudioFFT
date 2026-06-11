#include "MainWindow.h"

#include <QApplication>
#include <QIcon>
#include <QSettings>
#include <QTranslator>
#include <QDir>        
#include <QFileInfo>
#include <QLocale>
#include <QSurfaceFormat>
#include <QSharedMemory>
#include <QLocalSocket>
#include <QStringList>

extern "C" {
#include <libavformat/avformat.h>
}

QString detectSystemLanguage() {
    QLocale systemLocale = QLocale::system();
    QLocale::Language lang = systemLocale.language();
    switch (lang) {
        case QLocale::German: return "de";
        case QLocale::English: return "en";
        case QLocale::French: return "fr";
        case QLocale::Japanese: return "ja";
        case QLocale::Korean: return "ko";
        case QLocale::Russian: return "ru";
        case QLocale::Chinese:
            if (systemLocale.script() == QLocale::SimplifiedChineseScript || 
                systemLocale.country() == QLocale::China) {
                return "zh-JT";
            }
            return "zh-FT";
        default: return "";
    }
}

int main(int argc, char *argv[]) {
    QSurfaceFormat fmt = QSurfaceFormat::defaultFormat();
    fmt.setAlphaBufferSize(8);
    QSurfaceFormat::setDefaultFormat(fmt);
    QApplication a(argc, argv);
    avformat_network_init();
    QString configPath = QCoreApplication::applicationDirPath() + "/AudioFFT_Config.ini";
    QSettings settings(configPath, QSettings::IniFormat);
    settings.beginGroup("GlobalPreferences");
    bool allowMulti = settings.value("allowMultipleInstances", true).toBool();
    settings.endGroup();
    QStringList arguments = QApplication::arguments();
    QString externalFile;
    if (arguments.size() > 1) {
        externalFile = arguments[1];
    }
    QSharedMemory sharedMemory("AudioFFT_SingleInstance_SharedMemory");
    if (!allowMulti) {
        QLocalSocket socket;
        socket.connectToServer("AudioFFT_SingleInstance_Server");
        if (socket.waitForConnected(500)) {
            if (!externalFile.isEmpty()) {
                socket.write(externalFile.toUtf8());
                socket.waitForBytesWritten(500);
            }
            return 0;
        }
        if (!sharedMemory.create(1)) {
        }
    }
    a.setWindowIcon(QIcon(":/AudioFFT_logo.ico")); 
    QString langCode = settings.value("language").toString();
    bool isAutoDetect = false;
    if (langCode.isEmpty()) {
        langCode = detectSystemLanguage();
        isAutoDetect = true;
    }
    QTranslator translator; 
    bool loadSuccess = false;
    if (!langCode.isEmpty()) {
        QString qmPath = QDir(QCoreApplication::applicationDirPath())
                         .filePath(QString("translations/AudioFFT_%1.qm").arg(langCode));
        if (QFileInfo::exists(qmPath) && translator.load(qmPath)) {
            a.installTranslator(&translator);
            loadSuccess = true;
        }
    }
    if (!loadSuccess && isAutoDetect) {
        langCode = ""; 
    }
    MainWindow w(langCode, externalFile); 
    w.show();
    return a.exec();
}
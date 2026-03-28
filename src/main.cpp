#include "MainWindow.h"

#include <QApplication>
#include <QIcon>
#include <QSettings>
#include <QTranslator>
#include <QDir>        
#include <QFileInfo>
#include <QLocale>

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
    QApplication a(argc, argv);
    avformat_network_init();
    a.setWindowIcon(QIcon(":/AudioFFT_logo.ico")); 
    QString configPath = QCoreApplication::applicationDirPath() + "/AudioFFT_Config.ini";
    QSettings settings(configPath, QSettings::IniFormat);
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
    MainWindow w(langCode); 
    w.show();
    return a.exec();
}
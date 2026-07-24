#include "ColorUserPaletteLoader.h"
#include "ColorPaletteParser.h"
#include "ColorPaletteFactory.h"

#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QTextStream>

void ColorUserPaletteLoader::loadUserPalettes(std::function<void(const QString&)> logger) {
    QString colorsDir = QCoreApplication::applicationDirPath() + "/Colors";
    QDir dir(colorsDir);
    if (!dir.exists()) {
        if (dir.mkpath(".")) {
            createReadmeIfNotExist(colorsDir);
            if (logger) logger(QString("[色彩加载] 自动创建了用户色彩文件夹：%1").arg(colorsDir));
        } else {
            if (logger) logger("[色彩加载] 错误：无法创建 Colors 文件夹。");
            return;
        }
    } else {
        createReadmeIfNotExist(colorsDir);
    }
    QStringList filters;
    filters << "User-*.txt";
    QFileInfoList fileList = dir.entryInfoList(filters, QDir::Files | QDir::NoDotAndDotDot);
    int successCount = 0;
    for (const QFileInfo& fileInfo : fileList) {
        QString errorMsg;
        auto palette = ColorPaletteParser::parse(fileInfo.absoluteFilePath(), errorMsg);
        if (palette) {
            ColorPaletteFactory::instance().registerPalette(palette);
            successCount++;
            if (logger) logger(QString("[色彩加载] 成功挂载自定义色彩：[%1] %2")
                               .arg(palette->getId()).arg(palette->getName()));
        } else {
            if (logger) logger(QString("[色彩加载] 跳过文件 %1：%2")
                               .arg(fileInfo.fileName()).arg(errorMsg));
        }
    }
    if (logger && successCount > 0) {
        logger(QString("[色彩加载] 共加载 %1 个自定义色彩方案。").arg(successCount));
    }
}

void ColorUserPaletteLoader::createReadmeIfNotExist(const QString& dirPath) {
    QString readmePath = dirPath + "/README.txt";
    if (QFile::exists(readmePath)) return;
    QFile file(readmePath);
    if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QTextStream out(&file);
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
        out.setEncoding(QStringConverter::Utf8);
#else
        out.setCodec("UTF-8");
#endif
        out << QString::fromUtf8("====== AudioFFT 自定义色彩说明 ======\n\n");
        out << QString::fromUtf8("您可以创建名为 User-xxx.txt 的文件来自定义色彩方案。\n\n");
        out << QString::fromUtf8("【模式1：linear (线性插值)】\n");
        out << QString::fromUtf8("我的插值颜色           <- 第一行：色彩名称\n");
        out << QString::fromUtf8("linear                 <- 第二行：模式标志位\n");
        out << QString::fromUtf8("0,   30,  0,   0       <- 第三行起：(进度,R,G,B)\n");
        out << QString::fromUtf8("0.5, 180, 100, 100     <- 进度必须在 0.0 ~ 1.0 之间\n");
        out << QString::fromUtf8("1,   255, 255, 255     <- 程序会自动平滑过渡补齐256色\n\n");
        out << QString::fromUtf8("【模式2：scientific (256色指定)】\n");
        out << QString::fromUtf8("我的科学颜色           <- 第一行：色彩名称\n");
        out << QString::fromUtf8("scientific             <- 第二行：模式标志位\n");
        out << QString::fromUtf8("000000                 <- 第三行起：必须严格完整提供 256 行 16 进制颜色\n");
        out << QString::fromUtf8("FFFFFF                 <- RRGGBB 格式\n");
        file.close();
    }
}
#pragma once

#include "AudioExtensionList.h"
#include "WindowFunctions.h"
#include "MappingCurves.h"
#include "ColorPalette.h"

#include <QString>
#include <QDateTime>
#include <QStringList>
#include <QCoreApplication>

namespace BatUtils {
    inline QString getCurrentTimestamp() {
        return QDateTime::currentDateTime().toString("[yyyyMMdd hh:mm:ss.zzz]");
    }
    inline QString formatSize(qint64 bytes) {
        if (bytes < 0) return "N/A";
        if (bytes >= 1024 * 1024 * 1024) return QString::number(bytes / (1024.0 * 1024 * 1024.0), 'f', 2) + " GB";
        if (bytes >= 1024 * 1024) return QString::number(bytes / (1024.0 * 1024.0), 'f', 2) + " MB";
        if (bytes >= 1024) return QString::number(bytes / 1024.0, 'f', 2) + " KB";
        return QString::number(bytes) + " B";
    }
    inline QString formatPreciseDuration(double total_seconds) {
        if (total_seconds < 0) return "00:00.000";
        long long total_milliseconds = static_cast<long long>(total_seconds * 1000.0);
        long long milliseconds = total_milliseconds % 1000;
        long long total_secs = total_milliseconds / 1000;
        long long seconds = total_secs % 60;
        long long total_mins = total_secs / 60;
        long long minutes = total_mins % 60;
        long long hours = total_mins / 60;
        if (hours > 0) {
            return QString("%1:%2:%3.%4")
                .arg(hours)
                .arg(minutes, 2, 10, QChar('0'))
                .arg(seconds, 2, 10, QChar('0'))
                .arg(milliseconds, 3, 10, QChar('0'));
        } else {
            return QString("%1:%2.%3")
                .arg(minutes, 2, 10, QChar('0'))
                .arg(seconds, 2, 10, QChar('0'))
                .arg(milliseconds, 3, 10, QChar('0'));
        }
    }
    inline QString formatElapsedMs(qint64 ms) {
        if (ms < 0) return "00:00.000";
        qint64 milliseconds = ms % 1000;
        qint64 total_seconds = ms / 1000;
        qint64 seconds = total_seconds % 60;
        qint64 total_minutes = total_seconds / 60;
        qint64 minutes = total_minutes % 60;
        qint64 hours = total_minutes / 60;
        if (hours > 0) {
            return QString("%1:%2:%3.%4")
                .arg(hours)
                .arg(minutes, 2, 10, QChar('0'))
                .arg(seconds, 2, 10, QChar('0'))
                .arg(milliseconds, 3, 10, QChar('0'));
        } else {
            return QString("%1:%2.%3")
                .arg(minutes)
                .arg(seconds, 2, 10, QChar('0'))
                .arg(milliseconds, 3, 10, QChar('0'));
        }
    }
template <typename SettingsType>
    inline QString formatSettings(const SettingsType& settings, bool isFullLoadMode) {
        QStringList lines;
        lines << QString("    [模式] %1").arg(isFullLoadMode ? QCoreApplication::translate("BatUtils", "全量") : QCoreApplication::translate("BatUtils", "流式"));
        QString threadStr = (settings.threadCount > 0) ? QString::number(settings.threadCount) : QCoreApplication::translate("BatUtils", "自动");
        lines << QString("    [线程] %1").arg(threadStr);
        QStringList inputOpts;
        if (settings.includeSubfolders) inputOpts << QCoreApplication::translate("BatUtils", "扫描子文件夹");
        if (settings.enableWhitelist) inputOpts << QCoreApplication::translate("BatUtils", "仅扫描白名单");
        if (settings.excludeVideoFiles) inputOpts << QCoreApplication::translate("BatUtils", "排除视频文件");
        lines << QString("    [输入] %1").arg(inputOpts.isEmpty() ? QCoreApplication::translate("BatUtils", "未设置") : inputOpts.join(" | "));
        if (settings.enableWhitelist) {
            lines << QCoreApplication::translate("BatUtils", "        扩展名白名单");
            QStringList builtinExts = AudioExtensionList::getDefaultExtensions();
            QStringList selectedBuiltin;
            QStringList selectedCustom;
            for (const QString& ext : settings.whitelistExtensions) {
                if (builtinExts.contains(ext, Qt::CaseInsensitive)) {
                    selectedBuiltin << ext;
                } else {
                    selectedCustom << ext;
                }
            }
            auto wrapExtensions = [](const QStringList& exts) -> QString {
                if (exts.isEmpty()) return QCoreApplication::translate("BatUtils", "无");
                QStringList chunks;
                for (int i = 0; i < exts.size(); i += 20) {
                    chunks << exts.mid(i, 20).join(", ");
                }
                return chunks.join(",\n                ");
            };
            lines << QString("        内置：%1").arg(wrapExtensions(selectedBuiltin));
            lines << QString("        自定义：%1").arg(wrapExtensions(selectedCustom));
        }
        QStringList outputOpts;
        if (settings.reuseSubfolderStructure) outputOpts << QCoreApplication::translate("BatUtils", "保持输入目录的层级结构");
        if (settings.categorizeByCodec) outputOpts << QCoreApplication::translate("BatUtils", "按音频编码类型进行分类");
        lines << QString("    [输出] %1").arg(outputOpts.isEmpty() ? QCoreApplication::translate("BatUtils", "未设置") : outputOpts.join(" | "));
        QString precisionStr = (settings.timeInterval <= 0.000000001) ? QCoreApplication::translate("BatUtils", "自动") : QString::number(settings.timeInterval) + QCoreApplication::translate("BatUtils", " 秒");
        lines << QString("    [参数] 高度：%1 | 精度：%2 | 窗口：%3 | 映射：%4 | 配色：%5 | dB：%6 ~ %7")
            .arg(settings.imageHeight)
            .arg(precisionStr)
            .arg(WindowFunctions::getName(settings.windowType))
            .arg(MappingCurves::getName(settings.curveType))
            .arg(ColorPalette::getPaletteNames().value(settings.paletteType, QCoreApplication::translate("BatUtils", "未知")))
            .arg(settings.minDb).arg(settings.maxDb);
        QString gridStr = settings.enableGrid ? QCoreApplication::translate("BatUtils", "启用") : QCoreApplication::translate("BatUtils", "关闭");
        QString compStr = settings.enableComponents ? QCoreApplication::translate("BatUtils", "启用") : QCoreApplication::translate("BatUtils", "关闭");
        QString widthStr = settings.enableWidthLimit ? QString::number(settings.maxWidth) : QCoreApplication::translate("BatUtils", "关闭");
        lines << QString("    [辅助] 网格：%1 | 组件：%2 | 限宽：%3").arg(gridStr, compStr, widthStr);
        QString qualityType = QCoreApplication::translate("BatUtils", "质量");
        if (settings.exportFormat == "PNG" || settings.exportFormat == "QtPNG") {
            qualityType = QCoreApplication::translate("BatUtils", "压缩");
        }
        QString qualityVal;
        if (settings.exportFormat == "QtPNG" || settings.exportFormat == "BMP" || settings.exportFormat == "TIFF") {
            qualityVal = QCoreApplication::translate("BatUtils", "无损");
        } else {
            qualityVal = QString::number(settings.qualityLevel);
        }
        lines << QString("    [图像] 格式：%1 | %2：%3").arg(settings.exportFormat, qualityType, qualityVal);
        return lines.join("\n");
    }
}
#pragma once

#include <QString>
#include <QDateTime>


namespace BatUtils {

    inline QString getCurrentTimestamp() {
        return QDateTime::currentDateTime().toString("[yyyyMMdd hh:mm:ss.zzz]");}

    inline QString formatSize(qint64 bytes) {
        if (bytes < 0) return "N/A";
        if (bytes >= 1024 * 1024 * 1024) return QString::number(bytes / (1024.0 * 1024 * 1024.0), 'f', 2) + " GB";
        if (bytes >= 1024 * 1024) return QString::number(bytes / (1024.0 * 1024.0), 'f', 2) + " MB";
        if (bytes >= 1024) return QString::number(bytes / 1024.0, 'f', 2) + " KB";
        return QString::number(bytes) + " B";}

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
                .arg(milliseconds, 3, 10, QChar('0'));}
            else {
                return QString("%1:%2.%3")
                    .arg(minutes, 2, 10, QChar('0'))
                    .arg(seconds, 2, 10, QChar('0'))
                    .arg(milliseconds, 3, 10, QChar('0'));}}

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
                .arg(milliseconds, 3, 10, QChar('0'));}
            else {
                return QString("%1:%2.%3")
                    .arg(minutes)
                    .arg(seconds, 2, 10, QChar('0'))
                    .arg(milliseconds, 3, 10, QChar('0'));}}

} // namespace BatUtils
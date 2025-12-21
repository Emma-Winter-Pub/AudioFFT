#pragma once

#include <QString>
#include <QDateTime>


inline QString getCurrentTimestamp() {
    return QDateTime::currentDateTime().toString("[yyyyMMdd hh:mm:ss.zzz]");
}


inline QString formatSize(qint64 bytes) {
    if (bytes >= 1024 * 1024 * 1024) return QString::number(bytes / (1024.0 * 1024 * 1024.0), 'f', 2) + "GB";
    if (bytes >= 1024 * 1024) return QString::number(bytes / (1024.0 * 1024.0), 'f', 2) + "MB";
    if (bytes >= 1024) return QString::number(bytes / 1024.0, 'f', 2) + "KB";
    return QString::number(bytes) + "B";
}


inline QString formatPreciseDuration(long long microseconds) {
    if (microseconds <= 0) return QString();
    long long totalMilliseconds = microseconds / 1000;
    int milliseconds = totalMilliseconds % 1000;
    long long totalSeconds = totalMilliseconds / 1000;
    int seconds = totalSeconds % 60;
    long long totalMinutes = totalSeconds / 60;
    int minutes = totalMinutes % 60;
    long long hours = totalMinutes / 60;

    QString formattedTime;
    if (hours > 0) {
        formattedTime = QString("%1:%2:%3.%4")
            .arg(hours)
            .arg(minutes, 2, 10, QChar('0'))
            .arg(seconds, 2, 10, QChar('0'))
            .arg(milliseconds, 3, 10, QChar('0'));}
    else {
        formattedTime = QString("%1:%2.%3")
            .arg(minutes)
            .arg(seconds, 2, 10, QChar('0'))
            .arg(milliseconds, 3, 10, QChar('0'));}
    return formattedTime;
}


inline QString formatElapsedMsAsStopwatch(int ms) {
    if (ms < 0) return QStringLiteral("0:00.000");

    int milliseconds = ms % 1000;
    int total_seconds = ms / 1000;
    int seconds = total_seconds % 60;
    int total_minutes = total_seconds / 60;
    int minutes = total_minutes % 60;
    int hours = total_minutes / 60;

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
            .arg(milliseconds, 3, 10, QChar('0'));}
}
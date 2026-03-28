#pragma once

#include <QString>
#include <QVector>
#include <optional>

struct CueTrack {
    int number = 0;
    QString title;
    QString performer;
    QString startTimeStr;
    double startSeconds = 0.0;
    double endSeconds = 0.0;
};

struct CueSheet {
    QString filePath;
    QString albumTitle;
    QString albumPerformer;
    QString audioFilename;
    QString year;
    QVector<CueTrack> tracks;
    bool isValid() const { return !tracks.isEmpty(); }
};

class CueParser {
public:
    static std::optional<CueSheet> parse(const QString& cueFilePath);
    static double parseCueTime(const QString& timeStr);

private:
    static QString extractQuoted(const QString& line);
    static QString detectEncodingAndRead(const QByteArray& data);
};
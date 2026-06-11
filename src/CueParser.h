#pragma once

#include <QString>
#include <QVector>
#include <optional>
#include <cstdint>

struct CueTrack {
    int number = 0;
    QString title;
    QString performer;
    QString audioFilename; 
    QString startTimeStr;
    int64_t index00Frames = -1;
    int64_t index01Frames = 0;
    int64_t endFrames = -1;
    double getStartSeconds() const { return index01Frames / 75.0; }
    double getEndSeconds() const { return endFrames >= 0 ? (endFrames / 75.0) : 0.0; }
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
    static int64_t parseCueTime(const QString& timeStr);

private:
    static QString extractQuoted(const QString& line);
    static QString detectEncodingAndRead(const QByteArray& data);
};
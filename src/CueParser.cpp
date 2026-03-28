#include "CueParser.h"

#include <QFile>
#include <QRegularExpression>
#include <QFileInfo>
#include <QStringDecoder>
#include <QDebug>

namespace {
    constexpr double FRAMES_PER_SEC = 75.0;
}

double CueParser::parseCueTime(const QString& timeStr) {
    static QRegularExpression re(R"((\d+):(\d+):(\d+))");
    auto match = re.match(timeStr);
    if (!match.hasMatch()) return 0.0;
    double min = match.captured(1).toDouble();
    double sec = match.captured(2).toDouble();
    double frame = match.captured(3).toDouble();
    return min * 60.0 + sec + (frame / FRAMES_PER_SEC);
}

QString CueParser::extractQuoted(const QString& line) {
    int first = line.indexOf('"');
    int last = line.lastIndexOf('"');
    if (first != -1 && last != -1 && last > first) {
        return line.mid(first + 1, last - first - 1);
    }
    int space = line.indexOf(' ');
    if (space != -1) return line.mid(space + 1).trimmed();
    return QString();
}

QString CueParser::detectEncodingAndRead(const QByteArray& data) {
    QStringDecoder decoderUtf8(QStringDecoder::Utf8);
    QString content = decoderUtf8.decode(data);
    if (!decoderUtf8.hasError() && content.count(QChar(0xFFFD)) == 0) {
        return content;
    }
    QStringDecoder decoderSystem(QStringDecoder::System);
    content = decoderSystem.decode(data);
    return content;
}

std::optional<CueSheet> CueParser::parse(const QString& cueFilePath) {
    QFile file(cueFilePath);
    if (!file.open(QIODevice::ReadOnly)) {
        return std::nullopt;
    }
    QByteArray data = file.readAll();
    file.close();
    QString content = detectEncodingAndRead(data);
    if (content.isEmpty()) return std::nullopt;
    CueSheet sheet;
    sheet.filePath = cueFilePath;
    QStringList lines = content.split('\n');
    bool inTrack = false;
    CueTrack currentTrack;
    QRegularExpression reTrack(R"(^\s*TRACK\s+(\d+)\s+AUDIO)", QRegularExpression::CaseInsensitiveOption);
    QRegularExpression reIndex(R"(^\s*INDEX\s+01\s+(\d+:\d+:\d+))", QRegularExpression::CaseInsensitiveOption);
    QRegularExpression reFile(R"(^\s*FILE\s+)", QRegularExpression::CaseInsensitiveOption);
    QRegularExpression reTitle(R"(^\s*TITLE\s+)", QRegularExpression::CaseInsensitiveOption);
    QRegularExpression rePerformer(R"(^\s*PERFORMER\s+)", QRegularExpression::CaseInsensitiveOption);
    QRegularExpression reRemDate(R"(^\s*REM\s+DATE\s+(\d{4}))", QRegularExpression::CaseInsensitiveOption);
    for (QString line : lines) {
        line = line.trimmed();
        if (line.isEmpty()) continue;
        auto matchTrack = reTrack.match(line);
        if (matchTrack.hasMatch()) {
            if (inTrack) {
                sheet.tracks.append(currentTrack);
            }
            inTrack = true;
            currentTrack = CueTrack();
            currentTrack.number = matchTrack.captured(1).toInt();
            continue;
        }
        if (!inTrack) {
            if (line.contains(reFile)) {
                sheet.audioFilename = extractQuoted(line);
            } else if (line.contains(reTitle)) {
                sheet.albumTitle = extractQuoted(line);
            } else if (line.contains(rePerformer)) {
                sheet.albumPerformer = extractQuoted(line);
            } else {
                auto matchDate = reRemDate.match(line);
                if (matchDate.hasMatch()) {
                    sheet.year = matchDate.captured(1);
                }
            }
        } else {
            if (line.contains(reTitle)) {
                currentTrack.title = extractQuoted(line);
            } else if (line.contains(rePerformer)) {
                currentTrack.performer = extractQuoted(line);
            } else if (line.contains(reIndex)) {
                auto matchIndex = reIndex.match(line);
                if (matchIndex.hasMatch()) {
                    currentTrack.startTimeStr = matchIndex.captured(1);
                    currentTrack.startSeconds = parseCueTime(currentTrack.startTimeStr);
                }
            }
        }
    }
    if (inTrack) {
        sheet.tracks.append(currentTrack);
    }
    for (int i = 0; i < sheet.tracks.size() - 1; ++i) {
        sheet.tracks[i].endSeconds = sheet.tracks[i+1].startSeconds;
    }
    if (sheet.tracks.isEmpty()) return std::nullopt;
    return sheet;
}
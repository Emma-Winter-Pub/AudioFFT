#include "CueParser.h"

#include <QFile>
#include <QRegularExpression>
#include <QFileInfo>
#include <QStringDecoder>

namespace {
    constexpr int64_t FRAMES_PER_SEC = 75;
}

int64_t CueParser::parseCueTime(const QString& timeStr) {
    static QRegularExpression re(R"((\d+):(\d+):(\d+))");
    auto match = re.match(timeStr);
    if (!match.hasMatch()) return -1;
    int64_t min = match.captured(1).toLongLong();
    int64_t sec = match.captured(2).toLongLong();
    int64_t frame = match.captured(3).toLongLong();
    if (sec >= 60 || frame >= FRAMES_PER_SEC) {
        return -1;
    }
    return min * 60 * FRAMES_PER_SEC + sec * FRAMES_PER_SEC + frame;
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
    return decoderSystem.decode(data);
}

std::optional<CueSheet> CueParser::parse(const QString& cueFilePath) {
    QFile file(cueFilePath);
    if (!file.open(QIODevice::ReadOnly)) return std::nullopt;
    QByteArray data = file.readAll();
    file.close();
    QString content = detectEncodingAndRead(data);
    if (content.isEmpty()) return std::nullopt;
    CueSheet sheet;
    sheet.filePath = cueFilePath;
    QStringList lines = content.split('\n');
    bool inTrack = false;
    CueTrack currentTrack;
    QString currentFile; 
    QRegularExpression reFile(R"(^\s*FILE\s+)", QRegularExpression::CaseInsensitiveOption);
    QRegularExpression reTrack(R"(^\s*TRACK\s+(\d+)\s+AUDIO)", QRegularExpression::CaseInsensitiveOption);
    QRegularExpression reIndex(R"(^\s*INDEX\s+(0[01])\s+(\d+:\d+:\d+))", QRegularExpression::CaseInsensitiveOption);
    QRegularExpression reTitle(R"(^\s*TITLE\s+)", QRegularExpression::CaseInsensitiveOption);
    QRegularExpression rePerformer(R"(^\s*PERFORMER\s+)", QRegularExpression::CaseInsensitiveOption);
    QRegularExpression reRemDate(R"(^\s*REM\s+DATE\s+(\d{4}))", QRegularExpression::CaseInsensitiveOption);
    for (QString line : lines) {
        line = line.trimmed();
        if (line.isEmpty()) continue;
        if (reFile.match(line).hasMatch()) {
            currentFile = extractQuoted(line);
            if (!inTrack && sheet.audioFilename.isEmpty()) {
                sheet.audioFilename = currentFile;
            }
        } 
        else if (auto matchTrack = reTrack.match(line); matchTrack.hasMatch()) {
            if (inTrack) {
                sheet.tracks.append(currentTrack);
            }
            inTrack = true;
            currentTrack = CueTrack();
            currentTrack.number = matchTrack.captured(1).toInt();
            currentTrack.audioFilename = currentFile; 
        }
        else if (auto matchIndex = reIndex.match(line); matchIndex.hasMatch()) {
            if (inTrack) {
                QString idxType = matchIndex.captured(1);
                QString timeStr = matchIndex.captured(2);
                int64_t frames = parseCueTime(timeStr);
                if (frames >= 0) {
                    if (idxType == "01") {
                        currentTrack.startTimeStr = timeStr;
                        currentTrack.index01Frames = frames;
                    } else if (idxType == "00") {
                        currentTrack.index00Frames = frames;
                    }
                }
            }
        }
        else if (reTitle.match(line).hasMatch()) {
            QString title = extractQuoted(line);
            if (inTrack) currentTrack.title = title;
            else sheet.albumTitle = title;
        }
        else if (rePerformer.match(line).hasMatch()) {
            QString perf = extractQuoted(line);
            if (inTrack) currentTrack.performer = perf;
            else sheet.albumPerformer = perf;
        }
        else if (auto matchDate = reRemDate.match(line); matchDate.hasMatch()) {
            if (!inTrack) sheet.year = matchDate.captured(1);
        }
    }
    if (inTrack) {
        sheet.tracks.append(currentTrack);
    }
    for (int i = 0; i < sheet.tracks.size() - 1; ++i) {
        if (sheet.tracks[i].audioFilename == sheet.tracks[i+1].audioFilename) {
            if (sheet.tracks[i+1].index00Frames > sheet.tracks[i].index01Frames) {
                sheet.tracks[i].endFrames = sheet.tracks[i+1].index00Frames;
            } else {
                sheet.tracks[i].endFrames = sheet.tracks[i+1].index01Frames;
            }
        } else {
            sheet.tracks[i].endFrames = -1;
        }
    }
    if (!sheet.tracks.isEmpty()) {
        sheet.tracks.last().endFrames = -1;
    }
    if (sheet.tracks.isEmpty()) return std::nullopt;
    return sheet;
}
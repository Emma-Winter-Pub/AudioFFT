#include "TextEncodingHelper.h"

#ifdef Q_OS_WIN
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#endif

#include <QStringDecoder>
#include <QLocale>
#include <algorithm>
#include <vector>

int TextEncodingHelper::scoreGB18030(const QByteArray& data) {
    int score = 0;
    int i = 0;
    while (i < data.size()) {
        unsigned char c = static_cast<unsigned char>(data[i]);
        if (c <= 0x7F) {
            score += 1;
            i += 1;
        } else {
            if (i + 1 >= data.size()) return -1;
            unsigned char c2 = static_cast<unsigned char>(data[i + 1]);
            
            if (c >= 0x81 && c <= 0xFE && c2 >= 0x40 && c2 <= 0xFE && c2 != 0x7F) {
                score += 10;
                i += 2;
            } 
            else if (c >= 0x81 && c <= 0xFE && c2 >= 0x30 && c2 <= 0x39) {
                if (i + 3 >= data.size()) return -1;
                unsigned char c3 = static_cast<unsigned char>(data[i + 2]);
                unsigned char c4 = static_cast<unsigned char>(data[i + 3]);
                if (c3 >= 0x81 && c3 <= 0xFE && c4 >= 0x30 && c4 <= 0x39) {
                    score += 20;
                    i += 4;
                } else {
                    return -1;
                }
            } else {
                return -1;
            }
        }
    }
    return score;
}

int TextEncodingHelper::scoreShiftJIS(const QByteArray& data) {
    int score = 0;
    int i = 0;
    while (i < data.size()) {
        unsigned char c = static_cast<unsigned char>(data[i]);
        if (c <= 0x7F) {
            score += 1;
            i += 1;
        } 
        else if (c >= 0xA1 && c <= 0xDF) {
            score += 2;
            i += 1;
        } else {
            if (i + 1 >= data.size()) return -1;
            unsigned char c2 = static_cast<unsigned char>(data[i + 1]);
            if (((c >= 0x81 && c <= 0x9F) || (c >= 0xE0 && c <= 0xFC)) &&
                ((c2 >= 0x40 && c2 <= 0x7E) || (c2 >= 0x80 && c2 <= 0xFC))) {
                score += 10;
                i += 2;
            } else {
                return -1;
            }
        }
    }
    return score;
}

int TextEncodingHelper::scoreBig5(const QByteArray& data) {
    int score = 0;
    int i = 0;
    while (i < data.size()) {
        unsigned char c = static_cast<unsigned char>(data[i]);
        if (c <= 0x7F) {
            score += 1;
            i += 1;
        } else {
            if (i + 1 >= data.size()) return -1;
            unsigned char c2 = static_cast<unsigned char>(data[i + 1]);
            if (c >= 0x81 && c <= 0xFE && ((c2 >= 0x40 && c2 <= 0x7E) || (c2 >= 0xA1 && c2 <= 0xFE))) {
                score += 10;
                i += 2;
            } else {
                return -1;
            }
        }
    }
    return score;
}

static QString decodeWithCodecFallback(const QByteArray& data, const char* codecName, unsigned int windowsCodePage) {
    QStringDecoder decoder(codecName);
    if (decoder.isValid()) {
        return decoder.decode(data);
    }

#ifdef Q_OS_WIN
    if (!data.isEmpty()) {
        int requiredChars = MultiByteToWideChar(windowsCodePage, 0, data.constData(), data.size(), nullptr, 0);
        if (requiredChars > 0) {
            std::vector<wchar_t> buffer(requiredChars);
            int written = MultiByteToWideChar(windowsCodePage, 0, data.constData(), data.size(), buffer.data(), requiredChars);
            if (written > 0) {
                return QString::fromWCharArray(buffer.data(), written);
            }
        }
    }
#endif

    return QStringDecoder(QStringDecoder::System).decode(data);
}

QString TextEncodingHelper::decode(const QByteArray& data) {
    if (data.isEmpty()) return QString();
    QStringDecoder utf8Decoder(QStringDecoder::Utf8);
    QString utf8Str = utf8Decoder.decode(data);
    if (!utf8Decoder.hasError() && !utf8Str.contains(QChar(0xFFFD))) {
        return utf8Str;
    }
    int gbScore = scoreGB18030(data);
    int sjisScore = scoreShiftJIS(data);
    int big5Score = scoreBig5(data);
    if (gbScore <= 0 && sjisScore <= 0 && big5Score <= 0) {
        return QStringDecoder(QStringDecoder::System).decode(data);
    }
    QLocale::Language lang = QLocale::system().language();
    if (lang == QLocale::Chinese) {
        if (QLocale::system().script() == QLocale::SimplifiedChineseScript || QLocale::system().country() == QLocale::China) {
            if (gbScore > 0) gbScore += 5;
        } else {
            if (big5Score > 0) big5Score += 5;
        }
    } else if (lang == QLocale::Japanese) {
        if (sjisScore > 0) sjisScore += 5;
    }
    int maxScore = std::max({gbScore, sjisScore, big5Score});
    if (maxScore == gbScore) {
        return decodeWithCodecFallback(data, "GB18030", 936);
    } else if (maxScore == sjisScore) {
        return decodeWithCodecFallback(data, "Shift_JIS", 932);
    } else {
        return decodeWithCodecFallback(data, "Big5", 950);
    }
}
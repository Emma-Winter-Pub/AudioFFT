#pragma once

#include <vector>
#include <QObject>
#include <QString>


struct AVFormatContext;


struct AVChannelLayout;


struct AudioMetadata {
    QString filePath;
    QString formatName;
    QString codecName;
    QString channelLayout;
    double durationSec = 0.0;
    long long bitRate = 0;
    int sampleRate = 0;
    bool isApe = false;
    qint64 fileSize = 0;
    long long preciseDurationMicroseconds = 0;
};


class AudioDecoder : public QObject {
    Q_OBJECT

public:
    explicit AudioDecoder(QObject *parent = nullptr);
    ~AudioDecoder();
    bool decode(const QString& filePath);
    const std::vector<float>& pcmData() const { return m_pcmData; }
    const AudioMetadata& metadata() const { return m_metadata; }

signals:
    void logMessage(const QString& message);

private:
    bool extractMetadata(AVFormatContext* formatContext);
    void logFileDetails(AVFormatContext* formatContext);
    bool decodeAudio(AVFormatContext* formatContext);
    static QString getChannelLayoutString(const AVChannelLayout* ch_layout);
    std::vector<float> m_pcmData;
    AudioMetadata m_metadata;
};
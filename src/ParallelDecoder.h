#pragma once

#include "AudioDecoder.h"

#include <QObject>
#include <QString>
#include <vector>


struct AVFormatContext;

class ParallelDecoder : public QObject {
    Q_OBJECT

public:
    explicit ParallelDecoder(QObject *parent = nullptr);

    static bool canHandle(const AudioMetadata& metadata);

    std::vector<float> decode(AVFormatContext* formatContext, const AudioMetadata& metadata);

signals:
    void logMessage(const QString& message);

private:
    std::vector<float> decodeApe(AVFormatContext* formatContext, const AudioMetadata& metadata);

};
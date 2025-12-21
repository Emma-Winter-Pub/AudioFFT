#pragma once

#include "AudioDecoder.h"

#include <QObject>
#include <QString>
#include <vector>


struct AVFormatContext;


class StandardDecoder : public QObject {

    Q_OBJECT

public:
    explicit StandardDecoder(QObject *parent = nullptr);
    std::vector<float> decode(AVFormatContext* formatContext, AudioMetadata& metadata);

signals:
    void logMessage(const QString& message);

};
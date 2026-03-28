#pragma once

#include <QImage>
#include <QString>
#include <QByteArray>

class IImageEncoder
{
public:
    virtual ~IImageEncoder() = default;
    virtual bool encodeAndSave(const QImage& image, const QString& filePath, int quality) const = 0;
    virtual QByteArray encodeToMemory(const QImage& image, int quality) const = 0;
};
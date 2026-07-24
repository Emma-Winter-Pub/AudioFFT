#pragma once

#include <QImage>
#include <QString>
#include <QByteArray>

class XImageEncoder
{
public:
    virtual ~XImageEncoder() = default;
    virtual bool EncodeAndSave(const QImage& image, const QString& filePath, int quality) const = 0;
    virtual QByteArray EncodeToMemory(const QImage& image, int quality) const = 0;
};
#pragma once

#include <QImage>
#include <QString>


class IImageEncoder {

public:
    virtual ~IImageEncoder() = default;
    virtual bool encodeAndSave(const QImage& image, const QString& filePath, int quality) const = 0;

};
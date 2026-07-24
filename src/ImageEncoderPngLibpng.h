#pragma once

#include "XImageEncoder.h"

class ImageEncoderPngLibpng : public XImageEncoder
{
public:
    bool EncodeAndSave(const QImage& image, const QString& filePath, int quality) const override;
    QByteArray EncodeToMemory(const QImage& image, int quality) const override;
};
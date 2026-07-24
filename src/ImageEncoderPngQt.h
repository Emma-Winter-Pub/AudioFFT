#pragma once

#include "XImageEncoder.h"

class ImageEncoderPngQt : public XImageEncoder
{
public:
    bool EncodeAndSave(const QImage& image, const QString& filePath, int quality) const override;
    QByteArray EncodeToMemory(const QImage& image, int quality) const override;
};
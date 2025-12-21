#pragma once

#include "IImageEncoder.h"


class Jp2ImageEncoder : public IImageEncoder {
public:
    bool encodeAndSave(const QImage& image, const QString& filePath, int quality) const override;
};
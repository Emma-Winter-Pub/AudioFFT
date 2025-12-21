#pragma once

#include "IImageEncoder.h"


class BmpImageEncoder : public IImageEncoder {

public:
    bool encodeAndSave(const QImage& image, const QString& filePath, int quality) const override;

};
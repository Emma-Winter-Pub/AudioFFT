#pragma once

#include "IImageEncoder.h"


class TiffImageEncoder : public IImageEncoder {

public:
    bool encodeAndSave(const QImage& image, const QString& filePath, int quality) const override;

};
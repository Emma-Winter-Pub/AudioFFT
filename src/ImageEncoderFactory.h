#pragma once

#include "IImageEncoder.h"

#include <memory>
#include <QString>


class ImageEncoderFactory {

public:
    static std::unique_ptr<IImageEncoder> createEncoder(const QString& formatIdentifier);

};
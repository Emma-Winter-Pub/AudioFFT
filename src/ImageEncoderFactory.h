#pragma once

#include "XImageEncoder.h"

#include <memory>
#include <QString>

class ImageEncoderFactory
{
public:
    static std::unique_ptr<XImageEncoder> createEncoder(const QString& formatIdentifier);
};
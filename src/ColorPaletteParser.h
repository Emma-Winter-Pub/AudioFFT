#pragma once

#include "XColorPalette.h"

#include <QString>
#include <memory>

class ColorPaletteParser {
public:
    static std::shared_ptr<XColorPalette> parse(const QString& filepath, QString& outError);
};
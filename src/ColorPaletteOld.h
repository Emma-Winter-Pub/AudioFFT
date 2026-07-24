#pragma once

#include "XColorPalette.h"

#include <vector>
#include <memory>
#include <QCoreApplication>

class ColorPaletteOld {
    Q_DECLARE_TR_FUNCTIONS(ColorPaletteOld)

public:
    static std::vector<std::shared_ptr<XColorPalette>> getLegacyPalettes();
};
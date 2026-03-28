#pragma once

#include "CrosshairOverlay.h"

class CrosshairConfig {
public:
    static CrosshairStyle load();
    static void save(const CrosshairStyle& style);
};
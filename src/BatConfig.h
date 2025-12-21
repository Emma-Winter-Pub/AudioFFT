#pragma once

#include <QColor>
#include <vector>

namespace BatConfig {

    constexpr double MIN_DB = -130.0;
    constexpr double MAX_DB = 0;

    constexpr int JPEG_MAX_DIMENSION       = 65535;
    constexpr int FINAL_IMAGE_TOP_MARGIN    = 30;
    constexpr int FINAL_IMAGE_BOTTOM_MARGIN = 30;
    constexpr int FINAL_IMAGE_LEFT_MARGIN   = 35;
    constexpr int FINAL_IMAGE_RIGHT_MARGIN  = 55;

    inline std::vector<QColor> getColorMap() {
        std::vector<QColor> colors;
        colors.reserve(256);
        QColor c;
        for (int i = 0; i < 256; ++i) {
            double norm = i / 255.0;
            if (norm < 0.25)               { c.setRgbF(0, 0, norm * 4.0);                                  }
                else if (norm < 0.5)       { c.setRgbF((norm - 0.25) * 4.0, 0, 1.0);                       }
                     else if (norm < 0.75) { c.setRgbF(1.0, (norm - 0.5) * 4.0, 1.0 - (norm - 0.5) * 4.0); }
                          else             { c.setRgbF(1.0, 1.0, (norm - 0.75) * 4.0);                     }
                          colors.push_back(c);}
        return colors;
    }

} // namespace BatConfig
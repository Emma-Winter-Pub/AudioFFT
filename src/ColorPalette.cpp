#include "ColorPalette.h"

#include <QColor>
#include <functional>
#include <map>
#include <algorithm>
#include <cmath>
#include <QtGlobal> 

struct RgbPoint { double pos; int r; int g; int b; };

static std::vector<QColor> interpolatePoints(const std::vector<RgbPoint>& points) {
    std::vector<QColor> colors;
    colors.reserve(256);    
    for (int i = 0; i < 256; ++i) {
        double pos = i / 255.0;
        const RgbPoint* p1 = &points.front();
        const RgbPoint* p2 = &points.back();        
        for (size_t j = 0; j < points.size() - 1; ++j) {
            if (pos >= points[j].pos && pos <= points[j+1].pos) {
                p1 = &points[j];
                p2 = &points[j+1];
                break;
            }
        }        
        double t = (p2->pos > p1->pos) ? (pos - p1->pos) / (p2->pos - p1->pos) : 0.0;
        int r = static_cast<int>(p1->r + t * (p2->r - p1->r));
        int g = static_cast<int>(p1->g + t * (p2->g - p1->g));
        int b = static_cast<int>(p1->b + t * (p2->b - p1->b));        
        r = std::max(0, std::min(255, r));
        g = std::max(0, std::min(255, g));
        b = std::max(0, std::min(255, b));
        colors.emplace_back(r, g, b);
    }
    return colors;
}

static std::vector<QColor> gen_S00() {
    std::vector<QColor> colors;
    colors.reserve(256);
    for (int i = 0; i < 256; ++i) {
        colors.emplace_back(i, i, i);
    }
    return colors;
}

static std::vector<QColor> gen_S01() {
    std::vector<QColor> colors;
    colors.reserve(256);
    QColor c;
    for (int i = 0; i < 256; ++i) {
        double norm = i / 255.0;
        if (norm < 0.25) {
            c.setRgbF(0, 0, norm * 4.0);
        } else if (norm < 0.5) {
            c.setRgbF((norm - 0.25) * 4.0, 0, 1.0);
        } else if (norm < 0.75) {
            c.setRgbF(1.0, (norm - 0.5) * 4.0, 1.0 - (norm - 0.5) * 4.0);
        } else {
            c.setRgbF(1.0, 1.0, (norm - 0.75) * 4.0);
        }
        colors.push_back(c);
    }
    return colors;
}

static std::vector<QColor> gen_S02() {
    return interpolatePoints({
        {0.00, 0, 0, 4}, {0.25, 80, 18, 123}, {0.50, 182, 55, 122}, {0.75, 251, 135, 97}, {1.00, 252, 253, 191}
    });
}

static std::vector<QColor> gen_S03() {
    return interpolatePoints({
        {0.00, 0, 0, 4}, {0.25, 87, 16, 110}, {0.50, 187, 55, 84}, {0.75, 249, 142, 9}, {1.00, 252, 255, 164}
    });
}

static std::vector<QColor> gen_S04() {
    return interpolatePoints({
        {0.00, 13, 8, 135}, {0.25, 126, 3, 168}, {0.50, 204, 71, 120}, {0.75, 248, 149, 64}, {1.00, 240, 249, 33}
    });
}

static std::vector<QColor> gen_S05() {
    return interpolatePoints({
        {0.00, 68, 1, 84}, {0.25, 59, 82, 139}, {0.50, 33, 145, 140}, {0.75, 94, 201, 98}, {1.00, 253, 231, 37}
    });
}

static std::vector<QColor> gen_S06() {
    return interpolatePoints({
        {0.00, 0, 32, 77}, {0.33, 107, 107, 112}, {0.66, 181, 179, 97}, {1.00, 255, 234, 70}
    });
}

static std::vector<QColor> gen_S07() {
    return interpolatePoints({
        {0.00, 48, 18, 59}, {0.15, 70, 107, 227}, {0.35, 23, 213, 179}, {0.55, 162, 252, 60}, {0.75, 251, 176, 33}, {0.90, 226, 62, 29}, {1.00, 122, 4, 3}
    });
}

static std::vector<QColor> gen_S08() {
    return interpolatePoints({
        {0.00, 0, 0, 0}, {0.30, 100, 0, 0}, {0.60, 255, 128, 0}, {0.85, 255, 255, 0}, {1.00, 255, 255, 255}
    });
}

static std::vector<QColor> gen_S09() {
    return interpolatePoints({
        {0.00, 0, 0, 0}, {0.35, 50, 50, 75}, {0.70, 160, 170, 180}, {1.00, 255, 255, 255}
    });
}

static std::vector<QColor> gen_S10() {
    return interpolatePoints({
        {0.00, 0, 0, 0}, {0.30, 0, 40, 80}, {0.60, 0, 120, 160}, {0.85, 100, 220, 200}, {1.00, 230, 255, 255}
    });
}

static std::vector<QColor> gen_S11() {
    return interpolatePoints({
        {0.00, 10, 15, 10}, {0.25, 30, 60, 30}, {0.50, 50, 120, 50}, {0.75, 140, 200, 80}, {1.00, 240, 250, 180}
    });
}

static std::vector<QColor> gen_S12() {
    return interpolatePoints({
        {0.00, 10, 0, 30}, {0.25, 80, 20, 100}, {0.50, 200, 40, 80}, {0.75, 255, 140, 0}, {1.00, 255, 230, 100}
    });
}

static std::vector<QColor> gen_S13() {
    return interpolatePoints({
        {0.00, 0, 10, 0}, {0.20, 0, 40, 0}, {0.60, 0, 160, 0}, {0.90, 50, 255, 50}, {1.00, 200, 255, 200}
    });
}

static std::vector<QColor> gen_S14() {
    return interpolatePoints({
        {0.00, 0, 0, 0}, {0.15, 0, 0, 255}, {0.30, 0, 255, 255}, {0.40, 0, 255, 0}, {0.60, 255, 255, 0}, {0.90, 255, 0, 0}, {1.00, 255, 255, 255}
    });
}

static std::vector<QColor> gen_S15() {
    return interpolatePoints({
        {0.00, 0, 0, 0}, {0.20, 33, 102, 172}, {0.40, 146, 197, 222}, {0.60, 247, 247, 247}, {0.80, 244, 165, 130}, {1.00, 178, 24, 43}
    });
}

static std::vector<QColor> gen_S16() {
    return interpolatePoints({
        {0.00, 0, 0, 0}, {0.05, 0, 0, 255}, {0.25, 0, 255, 255}, {0.50, 0, 255, 0}, {0.75, 255, 255, 0}, {1.00, 255, 0, 0}
    });
}

static std::vector<QColor> gen_S17() {
    return interpolatePoints({
        {0.00, 0, 0, 0}, {0.15, 0, 20, 100}, {0.30, 0, 120, 200}, {0.45, 34, 139, 34}, {0.60, 244, 164, 96}, {0.80, 139, 69, 19}, {0.95, 190, 190, 190}, {1.00, 255, 255, 255}
    });
}

static std::vector<QColor> gen_S18() {
    return interpolatePoints({
        {0.00, 0,   0,   128},
        {0.25, 0,   0,   255},
        {0.50, 0,   255, 0},
        {0.75, 255, 255, 0},
        {1.00, 255, 0,   0}
    });
}

static std::vector<QColor> gen_S19() {
    return interpolatePoints({
        {0.00, 0,   255, 255},
        {1.00, 255, 0,   255}
    });
}

static std::vector<QColor> gen_S20() {
    return interpolatePoints({
        {0.00, 0,   0,   0},
        {1.00, 255, 199, 127}
    });
}

static std::vector<QColor> gen_S21() {
    return interpolatePoints({
        {0.00, 30,  0,   0},
        {0.40, 180, 100, 100},
        {0.70, 230, 200, 180},
        {1.00, 255, 255, 255}
    });
}

static std::vector<QColor> gen_S22() {
    return interpolatePoints({
        {0.00, 225, 217, 226},
        {0.25, 65,  65,  200},
        {0.50, 10,  10,  10},
        {0.75, 160, 50,  50},
        {1.00, 225, 217, 226}
    });
}

static std::vector<QColor> gen_S23() {
    return interpolatePoints({
        {0.00, 0,   0,   0},
        {0.50, 255, 0,   0},
        {1.00, 255, 255, 255}
    });
}

static std::vector<QColor> gen_S24() {
    return interpolatePoints({
        {0.00, 0,   0,   0},
        {0.50, 0,   255, 0},
        {1.00, 255, 255, 255}
    });
}

static std::vector<QColor> gen_S25() {
    return interpolatePoints({
        {0.00, 0,   0,   0},
        {0.50, 0,   0,   255},
        {1.00, 255, 255, 255}
    });
}

using PaletteGenerator = std::function<std::vector<QColor>()>;

struct PaletteEntry {
    const char* nameId;
    PaletteGenerator generator;
};

static const std::map<PaletteType, PaletteEntry> s_paletteRegistry = {
    { PaletteType::S00, { QT_TRANSLATE_NOOP("ColorPalette", "00 黑白"), gen_S00 } },
    { PaletteType::S01, { QT_TRANSLATE_NOOP("ColorPalette", "01 经典"), gen_S01 } },
    { PaletteType::S02, { QT_TRANSLATE_NOOP("ColorPalette", "02 岩浆"), gen_S02 } },
    { PaletteType::S03, { QT_TRANSLATE_NOOP("ColorPalette", "03 地狱"), gen_S03 } },
    { PaletteType::S04, { QT_TRANSLATE_NOOP("ColorPalette", "04 离子"), gen_S04 } },
    { PaletteType::S05, { QT_TRANSLATE_NOOP("ColorPalette", "05 幽蓝"), gen_S05 } },
    { PaletteType::S06, { QT_TRANSLATE_NOOP("ColorPalette", "06 暖黄"), gen_S06 } },
    { PaletteType::S07, { QT_TRANSLATE_NOOP("ColorPalette", "07 涡轮"), gen_S07 } },
    { PaletteType::S08, { QT_TRANSLATE_NOOP("ColorPalette", "08 金属"), gen_S08 } },
    { PaletteType::S09, { QT_TRANSLATE_NOOP("ColorPalette", "09 骨骼"), gen_S09 } },
    { PaletteType::S10, { QT_TRANSLATE_NOOP("ColorPalette", "10 深海"), gen_S10 } },
    { PaletteType::S11, { QT_TRANSLATE_NOOP("ColorPalette", "11 丛林"), gen_S11 } },
    { PaletteType::S12, { QT_TRANSLATE_NOOP("ColorPalette", "12 日落"), gen_S12 } },
    { PaletteType::S13, { QT_TRANSLATE_NOOP("ColorPalette", "13 雷达"), gen_S13 } },
    { PaletteType::S14, { QT_TRANSLATE_NOOP("ColorPalette", "14 彩虹"), gen_S14 } },
    { PaletteType::S15, { QT_TRANSLATE_NOOP("ColorPalette", "15 气动"), gen_S15 } },
    { PaletteType::S16, { QT_TRANSLATE_NOOP("ColorPalette", "16 应力"), gen_S16 } },
    { PaletteType::S17, { QT_TRANSLATE_NOOP("ColorPalette", "17 高程"), gen_S17 } },
    { PaletteType::S18, { QT_TRANSLATE_NOOP("ColorPalette", "18 喷射"), gen_S18 } },
    { PaletteType::S19, { QT_TRANSLATE_NOOP("ColorPalette", "19 极光"), gen_S19 } },
    { PaletteType::S20, { QT_TRANSLATE_NOOP("ColorPalette", "20 古铜"), gen_S20 } },
    { PaletteType::S21, { QT_TRANSLATE_NOOP("ColorPalette", "21 暮云"), gen_S21 } },
    { PaletteType::S22, { QT_TRANSLATE_NOOP("ColorPalette", "22 黎明"), gen_S22 } },
    { PaletteType::S23, { QT_TRANSLATE_NOOP("ColorPalette", "23 红"),   gen_S23 } },
    { PaletteType::S24, { QT_TRANSLATE_NOOP("ColorPalette", "24 绿"),   gen_S24 } },
    { PaletteType::S25, { QT_TRANSLATE_NOOP("ColorPalette", "25 蓝"),   gen_S25 } }
};

std::vector<QColor> ColorPalette::getPalette(PaletteType type) {
    auto it = s_paletteRegistry.find(type);
    if (it != s_paletteRegistry.end()) {
        return it->second.generator();
    }
    return gen_S01();
}

QMap<PaletteType, QString> ColorPalette::getPaletteNames() {
    QMap<PaletteType, QString> names;
    for (const auto& pair : s_paletteRegistry) {
        names[pair.first] = QCoreApplication::translate("ColorPalette", pair.second.nameId);
    }
    return names;
}
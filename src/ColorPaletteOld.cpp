#include "ColorPaletteOld.h"
#include "ColorPaletteEntity.h"

#include <array>
#include <cmath>
#include <algorithm>

namespace {
    struct RgbPoint { double pos; int r; int g; int b; };
    static std::array<uint32_t, 256> interpolatePoints(const std::vector<RgbPoint>& points) {
        std::array<uint32_t, 256> colors;
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
            colors[i] = 0xFF000000 | (r << 16) | (g << 8) | b;
        }
        return colors;
    }
    static std::array<uint32_t, 256> gen_L001() {
        std::array<uint32_t, 256> colors;
        for (int i = 0; i < 256; ++i) {
            double norm = i / 255.0;
            double r_f = 0.0, g_f = 0.0, b_f = 0.0;
            if (norm < 0.25) {
                b_f = norm * 4.0;
            } else if (norm < 0.5) {
                r_f = (norm - 0.25) * 4.0;
                b_f = 1.0;
            } else if (norm < 0.75) {
                r_f = 1.0;
                g_f = (norm - 0.5) * 4.0;
                b_f = 1.0 - (norm - 0.5) * 4.0;
            } else {
                r_f = 1.0;
                g_f = 1.0;
                b_f = (norm - 0.75) * 4.0;
            }
            int r = std::max(0, std::min(255, static_cast<int>(r_f * 255.0)));
            int g = std::max(0, std::min(255, static_cast<int>(g_f * 255.0)));
            int b = std::max(0, std::min(255, static_cast<int>(b_f * 255.0)));
            colors[i] = 0xFF000000 | (r << 16) | (g << 8) | b;
        }
        return colors;
    }
    static std::array<uint32_t, 256> gen_L002() {
        return interpolatePoints({
            {0.00, 0,   0,   4  }, 
            {0.25, 80,  18,  123},
            {0.50, 182, 55,  122},
            {0.75, 251, 135, 97 },
            {1.00, 252, 253, 191}
        });
    }
    static std::array<uint32_t, 256> gen_L003() {
        return interpolatePoints({
            {0.00, 0,   0,   4  },
            {0.25, 87,  16,  110},
            {0.50, 187, 55,  84 },
            {0.75, 249, 142, 9  },
            {1.00, 252, 255, 164}
        });
    }
    static std::array<uint32_t, 256> gen_L004() {
        return interpolatePoints({
            {0.00, 13,  8,   135},
            {0.25, 126, 3,   168},
            {0.50, 204, 71,  120},
            {0.75, 248, 149, 64 },
            {1.00, 240, 249, 33 }
        });
    }
    static std::array<uint32_t, 256> gen_L005() {
        return interpolatePoints({
            {0.00, 68,  1,   84 },
            {0.25, 59,  82,  139},
            {0.50, 33,  145, 140},
            {0.75, 94,  201, 98 },
            {1.00, 253, 231, 37 }
        });
    }
    static std::array<uint32_t, 256> gen_L006() {
        return interpolatePoints({
            {0.00, 0,   32,  77 },
            {0.33, 107, 107, 112},
            {0.66, 181, 179, 97 },
            {1.00, 255, 234, 70 }
        });
    }
    static std::array<uint32_t, 256> gen_L007() {
        return interpolatePoints({
            {0.00, 48,  18,  59 },
            {0.15, 70,  107, 227},
            {0.35, 23,  213, 179},
            {0.55, 162, 252, 60 },
            {0.75, 251, 176, 33 },
            {0.90, 226, 62,  29 },
            {1.00, 122, 4,   3  }
        });
    }
    static std::array<uint32_t, 256> gen_L008() {
        return interpolatePoints({
            {0.00, 0,   0,   0  },
            {0.30, 100, 0,   0  },
            {0.60, 255, 128, 0  },
            {0.85, 255, 255, 0  },
            {1.00, 255, 255, 255}
        });
    }
    static std::array<uint32_t, 256> gen_L009() {
        return interpolatePoints({
            {0.00, 0,   0,   0  },
            {0.35, 50,  50,  75 },
            {0.70, 160, 170, 180},
            {1.00, 255, 255, 255}
        });
    }
    static std::array<uint32_t, 256> gen_L010() {
        return interpolatePoints({
            {0.00, 0,   0,   0  },
            {0.30, 0,   40,  80 },
            {0.60, 0,   120, 160},
            {0.85, 100, 220, 200},
            {1.00, 230, 255, 255}
        });
    }
    static std::array<uint32_t, 256> gen_L011() {
        return interpolatePoints({
            {0.00, 10,  15,  10 },
            {0.25, 30,  60,  30 },
            {0.50, 50,  120, 50 },
            {0.75, 140, 200, 80 },
            {1.00, 240, 250, 180}
        });
    }
    static std::array<uint32_t, 256> gen_L012() {
        return interpolatePoints({
            {0.00, 10,  0,   30 },
            {0.25, 80,  20,  100},
            {0.50, 200, 40,  80 },
            {0.75, 255, 140, 0  },
            {1.00, 255, 230, 100}
        });
    }
    static std::array<uint32_t, 256> gen_L013() {
        return interpolatePoints({
            {0.00, 0,   10,  0  },
            {0.20, 0,   40,  0  },
            {0.60, 0,   160, 0  },
            {0.90, 50,  255, 50 },
            {1.00, 200, 255, 200}
        });
    }
    static std::array<uint32_t, 256> gen_L014() {
        return interpolatePoints({
            {0.00, 0,   0,   0  },
            {0.15, 0,   0,   255},
            {0.30, 0,   255, 255},
            {0.40, 0,   255, 0  },
            {0.60, 255, 255, 0  },
            {0.90, 255, 0,   0  },
            {1.00, 255, 255, 255}
        });
    }
    static std::array<uint32_t, 256> gen_L015() {
        return interpolatePoints({
            {0.00, 0,   0,   0  },
            {0.20, 33,  102, 172},
            {0.40, 146, 197, 222},
            {0.60, 247, 247, 247},
            {0.80, 244, 165, 130},
            {1.00, 178, 24,  43 }
        });
    }
    static std::array<uint32_t, 256> gen_L016() {
        return interpolatePoints({
            {0.00, 0,   0,   0  },
            {0.05, 0,   0,   255},
            {0.25, 0,   255, 255},
            {0.50, 0,   255, 0  },
            {0.75, 255, 255, 0  },
            {1.00, 255, 0,   0  }
        });
    }
    static std::array<uint32_t, 256> gen_L017() {
        return interpolatePoints({
            {0.00, 0,   0,   0  },
            {0.15, 0,   20,  100},
            {0.30, 0,   120, 200},
            {0.45, 34,  139, 34 },
            {0.60, 244, 164, 96 },
            {0.80, 139, 69,  19 },
            {0.95, 190, 190, 190},
            {1.00, 255, 255, 255}
        });
    }
    static std::array<uint32_t, 256> gen_L018() {
        return interpolatePoints({
            {0.00, 0,   0,   128},
            {0.25, 0,   0,   255},
            {0.50, 0,   255, 0  },
            {0.75, 255, 255, 0  },
            {1.00, 255, 0,   0  }
        });
    }
    static std::array<uint32_t, 256> gen_L019() {
        return interpolatePoints({
            {0.00, 0,   255, 255},
            {1.00, 255, 0,   255}
        });
    }
    static std::array<uint32_t, 256> gen_L020() {
        return interpolatePoints({
            {0.00, 0,   0,   0  },
            {1.00, 255, 199, 127}
        });
    }
    static std::array<uint32_t, 256> gen_L021() {
        return interpolatePoints({
            {0.00, 30,  0,   0  },
            {0.40, 180, 100, 100},
            {0.70, 230, 200, 180},
            {1.00, 255, 255, 255}
        });
    }
    static std::array<uint32_t, 256> gen_L022() {
        return interpolatePoints({
            {0.00, 225, 217, 226},
            {0.25, 65,  65,  200},
            {0.50, 10,  10,  10 },
            {0.75, 160, 50,  50 },
            {1.00, 225, 217, 226}
        });
    }
    static std::array<uint32_t, 256> gen_L023() {
        return interpolatePoints({
            {0.00, 0,   0,   0  },
            {0.50, 255, 0,   0  },
            {1.00, 255, 255, 255}
        });
    }
    static std::array<uint32_t, 256> gen_L024() {
        return interpolatePoints({
            {0.00, 0,   0,   0  },
            {0.50, 0,   255, 0  },
            {1.00, 255, 255, 255}
        });
    }
    static std::array<uint32_t, 256> gen_L025() {
        return interpolatePoints({
            {0.00, 0,   0,   0  },
            {0.50, 0,   0,   255},
            {1.00, 255, 255, 255}
        });
    }
}

std::vector<std::shared_ptr<XColorPalette>> ColorPaletteOld::getLegacyPalettes() {
    std::vector<std::shared_ptr<XColorPalette>> palettes;
    palettes.push_back(std::make_shared<ColorPaletteEntity>("L001", tr("经典"), gen_L001()));
    palettes.push_back(std::make_shared<ColorPaletteEntity>("L002", tr("岩浆"), gen_L002()));
    palettes.push_back(std::make_shared<ColorPaletteEntity>("L003", tr("地狱"), gen_L003()));
    palettes.push_back(std::make_shared<ColorPaletteEntity>("L004", tr("离子"), gen_L004()));
    palettes.push_back(std::make_shared<ColorPaletteEntity>("L005", tr("幽蓝"), gen_L005()));
    palettes.push_back(std::make_shared<ColorPaletteEntity>("L006", tr("暖黄"), gen_L006()));
    palettes.push_back(std::make_shared<ColorPaletteEntity>("L007", tr("涡轮"), gen_L007()));
    palettes.push_back(std::make_shared<ColorPaletteEntity>("L008", tr("金属"), gen_L008()));
    palettes.push_back(std::make_shared<ColorPaletteEntity>("L009", tr("骨骼"), gen_L009()));
    palettes.push_back(std::make_shared<ColorPaletteEntity>("L010", tr("深海"), gen_L010()));
    palettes.push_back(std::make_shared<ColorPaletteEntity>("L011", tr("丛林"), gen_L011()));
    palettes.push_back(std::make_shared<ColorPaletteEntity>("L012", tr("日落"), gen_L012()));
    palettes.push_back(std::make_shared<ColorPaletteEntity>("L013", tr("雷达"), gen_L013()));
    palettes.push_back(std::make_shared<ColorPaletteEntity>("L014", tr("彩虹"), gen_L014()));
    palettes.push_back(std::make_shared<ColorPaletteEntity>("L015", tr("气动"), gen_L015()));
    palettes.push_back(std::make_shared<ColorPaletteEntity>("L016", tr("应力"), gen_L016()));
    palettes.push_back(std::make_shared<ColorPaletteEntity>("L017", tr("高程"), gen_L017()));
    palettes.push_back(std::make_shared<ColorPaletteEntity>("L018", tr("喷射"), gen_L018()));
    palettes.push_back(std::make_shared<ColorPaletteEntity>("L019", tr("极光"), gen_L019()));
    palettes.push_back(std::make_shared<ColorPaletteEntity>("L020", tr("古铜"), gen_L020()));
    palettes.push_back(std::make_shared<ColorPaletteEntity>("L021", tr("暮云"), gen_L021()));
    palettes.push_back(std::make_shared<ColorPaletteEntity>("L022", tr("黎明"), gen_L022()));
    palettes.push_back(std::make_shared<ColorPaletteEntity>("L023", tr("红"),   gen_L023()));
    palettes.push_back(std::make_shared<ColorPaletteEntity>("L024", tr("绿"),   gen_L024()));
    palettes.push_back(std::make_shared<ColorPaletteEntity>("L025", tr("蓝"),   gen_L025()));
    return palettes;
}
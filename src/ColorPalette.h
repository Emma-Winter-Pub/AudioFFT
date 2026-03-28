#pragma once

#include <vector>
#include <QColor>
#include <QString>
#include <QMap>
#include <QCoreApplication>

enum class PaletteType {
    S00,S01,S02,S03,S04,S05,S06,S07,S08,S09,
    S10,S11,S12,S13,S14,S15,S16,S17,S18,S19,
    S20,S21,S22,S23,S24,S25
};

class ColorPalette {
    Q_DECLARE_TR_FUNCTIONS(ColorPalette)

public:
    static std::vector<QColor> getPalette(PaletteType type);
    static QMap<PaletteType, QString> getPaletteNames();
};
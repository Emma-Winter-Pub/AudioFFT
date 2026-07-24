#pragma once

#include "XColorPalette.h"

#include <array>
#include <cstdint>

class ColorPaletteEntity : public XColorPalette {
public:
    ColorPaletteEntity(QString id, QString name, const std::array<uint32_t, 256>& rawColors);
    ~ColorPaletteEntity() override = default;
    QString getId() const override { return m_id; }
    QString getName() const override { return m_name; }
    const QList<QRgb>& getColors(bool inverted, bool negative) const override;

private:
    QString m_id;
    QString m_name;
    QList<QRgb> m_colorsNone;
    QList<QRgb> m_colorsBarInverted;
    QList<QRgb> m_colorsNegative;
    QList<QRgb> m_colorsBarInvertedAndcolorsNegative;
};
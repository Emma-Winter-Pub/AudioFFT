#include "ColorPaletteEntity.h"

#include <algorithm>
#include <QColor>

ColorPaletteEntity::ColorPaletteEntity(QString id, QString name, const std::array<uint32_t, 256>& rawColors)
    : m_id(std::move(id)), m_name(std::move(name))
{
    m_colorsNone.reserve(256);
    m_colorsNegative.reserve(256);
    for (uint32_t c : rawColors) {
        QRgb normColor = static_cast<QRgb>(c);
        m_colorsNone.append(normColor);
        int r = 255 - qRed(normColor);
        int g = 255 - qGreen(normColor);
        int b = 255 - qBlue(normColor);
        int a = qAlpha(normColor);
        m_colorsNegative.append(qRgba(r, g, b, a));
    }
    m_colorsBarInverted = m_colorsNone;
    std::reverse(m_colorsBarInverted.begin(), m_colorsBarInverted.end());
    m_colorsBarInvertedAndcolorsNegative = m_colorsNegative;
    std::reverse(m_colorsBarInvertedAndcolorsNegative.begin(), m_colorsBarInvertedAndcolorsNegative.end());
}

const QList<QRgb>& ColorPaletteEntity::getColors(bool inverted, bool negative) const {
    if (negative) {
        return inverted ? m_colorsBarInvertedAndcolorsNegative : m_colorsNegative;
    } else {
        return inverted ? m_colorsBarInverted : m_colorsNone;
    }
}
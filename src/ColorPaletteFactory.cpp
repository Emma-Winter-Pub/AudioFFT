#include "ColorPaletteFactory.h"
#include "ColorPaletteEntity.h"
#include "GeneratedPalettes.h"
#include "ColorPaletteOld.h"

#include <algorithm>
#include <array>
#include <mutex>

ColorPaletteFactory& ColorPaletteFactory::instance() {
    static ColorPaletteFactory s_instance;
    return s_instance;
}

ColorPaletteFactory::ColorPaletteFactory() {
    setupFallback();
}

void ColorPaletteFactory::setupFallback() {
    m_fallbackNormal.reserve(256);
    m_fallbackNegative.reserve(256);
    for (int i = 0; i < 256; ++i) {
        m_fallbackNormal.append(qRgb(i, i, i));
        m_fallbackNegative.append(qRgb(255 - i, 255 - i, 255 - i));
    }
    m_fallbackInverted = m_fallbackNormal;
    std::reverse(m_fallbackInverted.begin(), m_fallbackInverted.end());
    m_fallbackInvertedNegative = m_fallbackNegative;
    std::reverse(m_fallbackInvertedNegative.begin(), m_fallbackInvertedNegative.end());
}

void ColorPaletteFactory::initialize() {
    std::unique_lock<std::shared_mutex> lock(m_mutex);
    if (m_initialized) return;
    for (const auto& data : getBuiltinPalettes()) {
        auto entity = std::make_shared<ColorPaletteEntity>(data.id, data.name, data.colors);
        m_registry.insert(data.id, entity);
    }
    for (const auto& legacyPalette : ColorPaletteOld::getLegacyPalettes()) {
        m_registry.insert(legacyPalette->getId(), legacyPalette);
    }
    std::array<uint32_t, 256> gray;
    for (uint32_t i = 0; i < 256; ++i) {
        gray[i] = 0xFF000000 | (i << 16) | (i << 8) | i;
    }
    m_registry.insert("0000", std::make_shared<ColorPaletteEntity>("0000", tr("线性灰度"), gray));
    m_initialized = true;
}

void ColorPaletteFactory::registerPalette(std::shared_ptr<XColorPalette> palette) {
    if (!palette) return;
    std::unique_lock<std::shared_mutex> lock(m_mutex);
    m_registry.insert(palette->getId(), palette);
}

const QList<QRgb>& ColorPaletteFactory::getPalette(const QString& id, bool inverted, bool negative) const {
    std::shared_lock<std::shared_mutex> lock(m_mutex);
    auto it = m_registry.find(id);
    if (it != m_registry.end() && it.value()) {
        return it.value()->getColors(inverted, negative);
    }
    it = m_registry.find("0000");
    if (it != m_registry.end() && it.value()) {
        return it.value()->getColors(inverted, negative);
    }
    if (negative) {
        return inverted ? m_fallbackInvertedNegative : m_fallbackNegative;
    } else {
        return inverted ? m_fallbackInverted : m_fallbackNormal;
    }
}

QList<QPair<QString, QString>> ColorPaletteFactory::getAvailablePalettes() const {
    std::shared_lock<std::shared_mutex> lock(m_mutex);
    QList<QPair<QString, QString>> list;
    list.reserve(m_registry.size());
    for (auto it = m_registry.constBegin(); it != m_registry.constEnd(); ++it) {
        list.append({ it.key(), it.value()->getName() });
    }
    std::sort(list.begin(), list.end(), [](const auto& a, const auto& b) {
        return a.first < b.first;
    });
    return list;
}
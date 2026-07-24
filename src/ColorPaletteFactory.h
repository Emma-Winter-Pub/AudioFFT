#pragma once

#include "XColorPalette.h"

#include <QHash>
#include <QPair>
#include <QList>
#include <QCoreApplication>
#include <memory>
#include <shared_mutex>

class ColorPaletteFactory {
    Q_DECLARE_TR_FUNCTIONS(ColorPaletteFactory)

public:
    static ColorPaletteFactory& instance();
    void initialize();
    void registerPalette(std::shared_ptr<XColorPalette> palette);
    const QList<QRgb>& getPalette(const QString& id, bool inverted = false, bool negative = false) const;
    QList<QPair<QString, QString>> getAvailablePalettes() const;

private:
    ColorPaletteFactory();
    ~ColorPaletteFactory() = default;
    ColorPaletteFactory(const ColorPaletteFactory&) = delete;
    ColorPaletteFactory& operator=(const ColorPaletteFactory&) = delete;
    void setupFallback();
    QHash<QString, std::shared_ptr<XColorPalette>> m_registry;
    QList<QRgb> m_fallbackNormal;
    QList<QRgb> m_fallbackInverted;
    QList<QRgb> m_fallbackNegative;
    QList<QRgb> m_fallbackInvertedNegative;
    mutable std::shared_mutex m_mutex;
    bool m_initialized = false;
};
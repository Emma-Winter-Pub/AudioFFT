#pragma once

#include <QString>
#include <QList>
#include <QRgb>

class XColorPalette {
public:
    virtual ~XColorPalette() = default;
    virtual QString getId() const = 0;
    virtual QString getName() const = 0;
    virtual const QList<QRgb>& getColors(bool inverted, bool negative) const = 0;
};
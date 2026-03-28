#pragma once

#include "GlobalPreferences.h"

#include <vector>
#include <QColor>
#include <QRect>

class QWidget;

class ISpectrumRenderer {
public:
    virtual ~ISpectrumRenderer() = default;
    virtual void setData(const std::vector<float>& data) = 0;
    virtual void clearData() = 0;
    virtual void setStyle(const QColor& color, int lineWidth, bool filled, int fillAlpha, SpectrumProfileType type) = 0;
    virtual void setDrawRect(const QRect& rect) = 0;
    virtual QWidget* getWidget() = 0;
};
#pragma once

#include "GlobalPreferences.h"

#include <vector>
#include <QColor>
#include <QRect>

class QWidget;

class XSpectrumRenderer {
public:
    virtual ~XSpectrumRenderer() = default;
    virtual void setData(const std::vector<float>& data) = 0;
    virtual void clearData() = 0;
    virtual void setStyle(const QColor& color, int lineWidth, bool filled, int fillAlpha, SpectrumProfileType type, SpectrumProfileDirection direction) = 0;
    virtual void setDrawRect(const QRect& rect) = 0;
    virtual QWidget* getWidget() = 0;
};
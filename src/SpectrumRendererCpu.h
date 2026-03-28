#pragma once

#include "ISpectrumRenderer.h"

#include <QWidget>

class SpectrumRendererCpu : public QWidget, public ISpectrumRenderer {
    Q_OBJECT

public:
    explicit SpectrumRendererCpu(QWidget* parent = nullptr);
    ~SpectrumRendererCpu() override = default;
    void setData(const std::vector<float>& data) override;
    void clearData() override;
    void setStyle(const QColor& color, int lineWidth, bool filled, int fillAlpha, SpectrumProfileType type) override;
    void setDrawRect(const QRect& rect) override;
    QWidget* getWidget() override { return this; }

protected:
    void paintEvent(QPaintEvent* event) override;

private:
    std::vector<float> m_data;
    QColor m_lineColor = Qt::yellow;
    int m_lineWidth = 1;
    bool m_isFilled = false;
    int m_fillAlpha = 50;
    SpectrumProfileType m_type = SpectrumProfileType::Line;
    QRect m_drawRect;
};
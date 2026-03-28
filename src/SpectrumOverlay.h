#pragma once

#include "GlobalPreferences.h"

#include <QWidget>
#include <QStackedLayout>
#include <vector>

class ISpectrumRenderer;

class SpectrumOverlay : public QWidget {
    Q_OBJECT

public:
    explicit SpectrumOverlay(QWidget *parent = nullptr);
    ~SpectrumOverlay() override;
    void setData(const std::vector<float>& data);
    void clearData();
    void setStyle(const QColor& color, int lineWidth, bool filled, int fillAlpha, SpectrumProfileType type);
    void setDrawRect(const QRect& rect);
    void setGpuEnabled(bool enabled);

protected:
    void moveEvent(QMoveEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;
    void showEvent(QShowEvent *event) override;
    void hideEvent(QHideEvent *event) override;
    bool eventFilter(QObject *watched, QEvent *event) override;

private:
    QStackedLayout* m_layout;
    ISpectrumRenderer* m_cpuRenderer;
    ISpectrumRenderer* m_gpuRenderer;
    std::vector<float> m_cachedData;
    struct Style {
        QColor color = Qt::yellow;
        int lineWidth = 1;
        bool filled = false;
        int fillAlpha = 50;
        SpectrumProfileType type = SpectrumProfileType::Line;
    } m_currentStyle;
    QRect m_currentRect;
    bool m_gpuEnabled = false;
    ISpectrumRenderer* currentRenderer();
    void updateGpuGeometry();
};
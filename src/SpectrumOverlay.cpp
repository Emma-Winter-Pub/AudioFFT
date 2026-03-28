#include "SpectrumOverlay.h"
#include "SpectrumRendererCpu.h"
#include "SpectrumRendererGpu.h"
#include "GlobalPreferences.h"

#include <QEvent>
#include <QApplication>

SpectrumOverlay::SpectrumOverlay(QWidget *parent)
    : QWidget(parent)
{
    setAttribute(Qt::WA_TransparentForMouseEvents);
    setAttribute(Qt::WA_TranslucentBackground);
    setAttribute(Qt::WA_NoSystemBackground);
    m_layout = new QStackedLayout(this);
    m_layout->setStackingMode(QStackedLayout::StackOne);
    m_layout->setContentsMargins(0, 0, 0, 0);
    m_cpuRenderer = new SpectrumRendererCpu(this);
    m_gpuRenderer = new SpectrumRendererGpu(this); 
    m_layout->addWidget(m_cpuRenderer->getWidget());
    if (window()) {
        window()->installEventFilter(this);
    }
    GlobalPreferences prefs = GlobalPreferences::load();
    setGpuEnabled(prefs.enableGpuAcceleration);
}

SpectrumOverlay::~SpectrumOverlay() {}

void SpectrumOverlay::setGpuEnabled(bool enabled) {
    m_gpuEnabled = enabled;
    QWidget* gpuWidget = m_gpuRenderer->getWidget();
    QWidget* cpuWidget = m_cpuRenderer->getWidget();
    if (enabled) {
        cpuWidget->hide();
        updateGpuGeometry();
        if (isVisible()) {
            gpuWidget->show();
        }
    } else {
        gpuWidget->hide();
        cpuWidget->show();
        m_layout->setCurrentWidget(cpuWidget);
    }
    currentRenderer()->setDrawRect(m_currentRect);
    currentRenderer()->setStyle(
        m_currentStyle.color, 
        m_currentStyle.lineWidth, 
        m_currentStyle.filled, 
        m_currentStyle.fillAlpha, 
        m_currentStyle.type
    );
    if (!m_cachedData.empty()) {
        currentRenderer()->setData(m_cachedData);
    } else {
        currentRenderer()->clearData();
    }
}

ISpectrumRenderer* SpectrumOverlay::currentRenderer() {
    return m_gpuEnabled ? m_gpuRenderer : m_cpuRenderer;
}

void SpectrumOverlay::setData(const std::vector<float>& data) {
    m_cachedData = data;
    currentRenderer()->setData(data);
}

void SpectrumOverlay::clearData() {
    m_cachedData.clear();
    currentRenderer()->clearData();
}

void SpectrumOverlay::setStyle(const QColor& color, int lineWidth, bool filled, int fillAlpha, SpectrumProfileType type) {
    m_currentStyle = {color, lineWidth, filled, fillAlpha, type};
    currentRenderer()->setStyle(color, lineWidth, filled, fillAlpha, type);
}

void SpectrumOverlay::setDrawRect(const QRect& rect) {
    m_currentRect = rect;
    currentRenderer()->setDrawRect(rect);
}

void SpectrumOverlay::updateGpuGeometry() {
    if (m_gpuEnabled && m_gpuRenderer) {
        QWidget* gpuWidget = m_gpuRenderer->getWidget();
        QPoint globalPos = mapToGlobal(QPoint(0, 0));
        gpuWidget->setGeometry(globalPos.x(), globalPos.y(), width(), height());
    }
}

void SpectrumOverlay::moveEvent(QMoveEvent *event) {
    QWidget::moveEvent(event);
    updateGpuGeometry();
}

void SpectrumOverlay::resizeEvent(QResizeEvent *event) {
    QWidget::resizeEvent(event);
    updateGpuGeometry();
}

void SpectrumOverlay::showEvent(QShowEvent *event) {
    QWidget::showEvent(event);
    if (m_gpuEnabled) {
        m_gpuRenderer->getWidget()->show();
        updateGpuGeometry();
    }
}

void SpectrumOverlay::hideEvent(QHideEvent *event) {
    QWidget::hideEvent(event);
    if (m_gpuEnabled) {
        m_gpuRenderer->getWidget()->hide();
    }
}

bool SpectrumOverlay::eventFilter(QObject *watched, QEvent *event) {
    if (watched == window()) {
        if (event->type() == QEvent::Move || event->type() == QEvent::Resize) {
            updateGpuGeometry();
        }
    }
    return QWidget::eventFilter(watched, event);
}
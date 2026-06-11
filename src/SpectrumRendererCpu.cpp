#include "SpectrumRendererCpu.h"

#include <QPainter>
#include <QPainterPath>
#include <QVector>
#include <algorithm>
#include <cmath>

SpectrumRendererCpu::SpectrumRendererCpu(QWidget* parent) 
    : QWidget(parent) 
{
    setAttribute(Qt::WA_TransparentForMouseEvents);
    setAttribute(Qt::WA_TranslucentBackground);
    setAttribute(Qt::WA_NoSystemBackground);
}

void SpectrumRendererCpu::setData(const std::vector<float>& data) {
    m_data = data;
    update();
}

void SpectrumRendererCpu::clearData() {
    if (!m_data.empty()) {
        m_data.clear();
        update();
    }
}

void SpectrumRendererCpu::setStyle(const QColor& color, int lineWidth, bool filled, int fillAlpha, SpectrumProfileType type, SpectrumProfileDirection direction) {
    m_lineColor = color;
    m_lineWidth = lineWidth;
    m_isFilled = filled;
    m_fillAlpha = fillAlpha;
    m_type = type;
    m_direction = direction;
    update();
}

void SpectrumRendererCpu::setDrawRect(const QRect& rect) {
    if (m_drawRect != rect) {
        m_drawRect = rect;
        update();
    }
}

void SpectrumRendererCpu::paintEvent(QPaintEvent* event) {
    Q_UNUSED(event);
    if (m_data.empty()) return;
    QRect r = m_drawRect.isValid() ? m_drawRect : rect();
    int w = r.width();
    int h = r.height();
    if (w <= 0 || h <= 0) return;
    int drawLen = (m_direction == SpectrumProfileDirection::Horizontal) ? w : h;
    std::vector<float> displayData; 
    size_t dataCount = m_data.size();
    if (dataCount > static_cast<size_t>(drawLen * 2)) {
        displayData.resize(drawLen);
        for (int i = 0; i < drawLen; ++i) {
            size_t idxStart = (static_cast<size_t>(i) * dataCount) / drawLen;
            size_t idxEnd = (static_cast<size_t>(i + 1) * dataCount) / drawLen;
            if (idxEnd > dataCount) idxEnd = dataCount;
            if (idxEnd <= idxStart) idxEnd = idxStart + 1;
            float maxVal = 0.0f;
            for (size_t k = idxStart; k < idxEnd; ++k) {
                if (m_data[k] > maxVal) maxVal = m_data[k];
            }
            displayData[i] = maxVal;
        }
    } else {
        displayData = m_data;
    }
    size_t count = displayData.size();
    if (count < 1) return;
    QPainter painter(this);
    painter.setClipRect(r);
    auto calcAmplitude = [&](float val, int fullSize) -> double {
        if (val < 0.0f) val = 0.0f;
        if (val > 1.0f) val = 1.0f;
        return val * fullSize;
    };
    if (m_type == SpectrumProfileType::Bar) {
        painter.setRenderHint(QPainter::Antialiasing, false);
        QVector<QRectF> bars;
        QVector<QLineF> tops;
        bars.reserve(count);
        tops.reserve(count);
        double dn = static_cast<double>(count);
        if (m_direction == SpectrumProfileDirection::Horizontal) {
            double dw = static_cast<double>(w);
            for (size_t i = 0; i < count; ++i) {
                float val = displayData[i];
                if (val <= 0.001f) continue;
                double leftF = (i * dw) / dn;
                double rightF = ((i + 1) * dw) / dn;
                double x = r.left() + leftF;
                double width = rightF - leftF;
                if (width < 1.0) width = 1.0;
                double height = calcAmplitude(val, h);
                double y = r.bottom() - height;
                bars.append(QRectF(x, y, width, height));
                tops.append(QLineF(x, y, x + width, y));
            }
        } else {
            double dh = static_cast<double>(h);
            for (size_t i = 0; i < count; ++i) {
                float val = displayData[i];
                if (val <= 0.001f) continue;
                double bottomF = (i * dh) / dn;
                double topF = ((i + 1) * dh) / dn;
                double y = r.bottom() - topF;
                double height = topF - bottomF;
                if (height < 1.0) height = 1.0;
                double width = calcAmplitude(val, w);
                bars.append(QRectF(r.left(), y, width, height));
                tops.append(QLineF(r.left() + width, y, r.left() + width, y + height));
            }
        }
        if (m_isFilled && !bars.isEmpty()) {
            QColor barColor = m_lineColor;
            barColor.setAlpha(m_fillAlpha); 
            painter.setBrush(barColor);
            painter.setPen(Qt::NoPen);
            painter.drawRects(bars);
        }
        if (!tops.isEmpty()) {
            QPen topPen(m_lineColor);
            topPen.setWidth(m_lineWidth);
            painter.setPen(topPen);
            painter.setBrush(Qt::NoBrush);
            painter.drawLines(tops);
        }
    } else {
        if (count < 2) return;
        painter.setRenderHint(QPainter::Antialiasing, true);
        QPainterPath path;

        if (m_direction == SpectrumProfileDirection::Horizontal) {
            path.moveTo(r.left(), r.bottom() - calcAmplitude(displayData[0], h));
            double xStep = static_cast<double>(w) / (count - 1);
            for (size_t i = 1; i < count; ++i) {
                double x = r.left() + i * xStep;
                double y = r.bottom() - calcAmplitude(displayData[i], h);
                path.lineTo(x, y);
            }
            if (m_isFilled) {
                QPainterPath fillPath = path;
                fillPath.lineTo(r.right(), r.bottom());
                fillPath.lineTo(r.left(), r.bottom());
                fillPath.closeSubpath();
                QColor fillColor = m_lineColor;
                fillColor.setAlpha(m_fillAlpha);
                painter.fillPath(fillPath, fillColor);
            }
        } else {
            path.moveTo(r.left() + calcAmplitude(displayData[0], w), r.bottom());
            double yStep = static_cast<double>(h) / (count - 1);
            for (size_t i = 1; i < count; ++i) {
                double y = r.bottom() - i * yStep;
                double x = r.left() + calcAmplitude(displayData[i], w);
                path.lineTo(x, y);
            }
            if (m_isFilled) {
                QPainterPath fillPath = path;
                fillPath.lineTo(r.left(), r.top());
                fillPath.lineTo(r.left(), r.bottom());
                fillPath.closeSubpath();
                QColor fillColor = m_lineColor;
                fillColor.setAlpha(m_fillAlpha);
                painter.fillPath(fillPath, fillColor);
            }
        }
        QPen pen(m_lineColor);
        pen.setWidth(m_lineWidth);
        pen.setJoinStyle(Qt::RoundJoin);
        painter.setPen(pen);
        painter.setBrush(Qt::NoBrush);
        painter.drawPath(path);
    }
}
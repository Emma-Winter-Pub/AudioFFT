#include "PlayerControlComponents.h"

#include <QPainter>
#include <QPainterPath>
#include <QEvent>

SymbolButton::SymbolButton(QWidget* parent) 
    : QAbstractButton(parent), m_baseColor(220, 220, 220)
{
    setFixedSize(20, 20);
    setCursor(Qt::PointingHandCursor);
}

void SymbolButton::setSymbolType(SymbolType type) {
    if (m_type != type) {
        m_type = type;
        update();
    }
}

void SymbolButton::setBaseColor(const QColor& color) {
    m_baseColor = color;
    update();
}

QSize SymbolButton::sizeHint() const {
    return QSize(20, 20);
}

void SymbolButton::enterEvent(QEnterEvent* event) {
    Q_UNUSED(event);
    m_isHovered = true;
    update();
}

void SymbolButton::leaveEvent(QEvent* event) {
    Q_UNUSED(event);
    m_isHovered = false;
    update();
}

void SymbolButton::paintEvent(QPaintEvent* event) {
    Q_UNUSED(event);
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    if (isDown()) {
        p.setBrush(QColor(255, 255, 255, 40)); 
        p.setPen(Qt::NoPen);
        p.drawRoundedRect(rect(), 2, 2); 
    } else if (m_isHovered) {
        p.setBrush(QColor(255, 255, 255, 20)); 
        p.setPen(Qt::NoPen);
        p.drawRoundedRect(rect(), 2, 2);
    }
    QColor iconColor = m_baseColor;
    if (isDown()) iconColor = Qt::white;       
    else if (m_isHovered) iconColor = Qt::white; 
    else if (!isEnabled()) iconColor = QColor(100, 100, 100);
    int padding = 4; 
    QRect drawRect = rect().adjusted(padding, padding, -padding, -padding);
    switch (m_type) {
        case SymbolType::Play:  drawPlay(p, drawRect, iconColor); break;
        case SymbolType::Pause: drawPause(p, drawRect, iconColor); break;
        case SymbolType::Stop:  drawStop(p, drawRect, iconColor); break;
    }
}

void SymbolButton::drawPlay(QPainter& p, const QRect& r, const QColor& c) {
    QPainterPath path;
    float x1 = r.left() + 1.0f; 
    float y1 = r.top();
    float x2 = r.right();    
    float y2 = r.center().y();
    float x3 = x1;           
    float y3 = r.bottom();
    path.moveTo(x1, y1);
    path.lineTo(x2, y2);
    path.lineTo(x3, y3);
    path.closeSubpath();
    p.fillPath(path, c);
}

void SymbolButton::drawPause(QPainter& p, const QRect& r, const QColor& c) {
    p.setBrush(c);
    p.setPen(Qt::NoPen);
    QRect bounds = r.adjusted(1, 1, -1, -1);
    int gap = 3;
    int h = bounds.height();
    int barWidth = (bounds.width() - gap) / 2;
    p.drawRect(bounds.left(), bounds.top(), barWidth, h);
    p.drawRect(bounds.left() + barWidth + gap, bounds.top(), barWidth, h);
}

void SymbolButton::drawStop(QPainter& p, const QRect& r, const QColor& c) {
    p.setBrush(c);
    p.setPen(Qt::NoPen);
    p.drawRect(r.adjusted(1, 1, -1, -1)); 
}
#include "CrosshairOverlay.h"

#include <QPainter>
#include <QPaintEvent>
#include <algorithm>

CrosshairOverlay::CrosshairOverlay(QWidget *parent)
    : QWidget(parent)
    , m_currentPos(0, 0)
    , m_isCursorVisible(false)
    , m_isPlayheadActive(false)
    , m_linesEnabled(true)
{
    setAttribute(Qt::WA_TransparentForMouseEvents);
    setAttribute(Qt::WA_TranslucentBackground);
    setAttribute(Qt::WA_NoSystemBackground);
}

CrosshairOverlay::~CrosshairOverlay(){}

void CrosshairOverlay::updatePosition(const QPoint& pos){
    if (m_currentPos == pos) return;
    m_currentPos = pos;
    update();
}

void CrosshairOverlay::setStyle(const CrosshairStyle& style){
    m_style = style;
    update();
}

void CrosshairOverlay::setPlayheadStyle(const PlayheadStyle& style) {
    m_playheadStyle = style;
    update();
}

void CrosshairOverlay::setCursorVisible(bool visible){
    if (m_isCursorVisible != visible) {
        m_isCursorVisible = visible;
        update();
    }
}

void CrosshairOverlay::setDrawBounds(const QRect& bounds){
    if (m_drawBounds != bounds) {
        m_drawBounds = bounds;
        update();
    }
}

void CrosshairOverlay::setPlayheadPosition(int x, bool active) {
    if (m_playheadX != x || m_isPlayheadActive != active) {
        m_playheadX = x;
        m_isPlayheadActive = active;
        update();
    }
}

void CrosshairOverlay::setIndicatorData(const QString& freq, const QString& time, const QString& db) {
    m_indicator.setData(freq, time, db);
    update();
}

void CrosshairOverlay::setIndicatorVisibility(bool showFreq, bool showTime, bool showDb) {
    m_indicator.setVisibility(showFreq, showTime, showDb);
    update();
}

void CrosshairOverlay::setLinesEnabled(bool enabled) {
    if (m_linesEnabled != enabled) {
        m_linesEnabled = enabled;
        update();
    }
}

void CrosshairOverlay::paintEvent(QPaintEvent *event){
    Q_UNUSED(event);
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing, true);
    if (m_isPlayheadActive && !m_drawBounds.isEmpty()) {
        if (m_playheadX >= m_drawBounds.left() && m_playheadX <= m_drawBounds.right()) {
            if (m_playheadStyle.visible) {
                QColor lineColor = m_playheadStyle.lineColor;
                QPen playheadPen(lineColor);
                playheadPen.setWidth(m_playheadStyle.lineWidth);
                painter.setPen(playheadPen);
                painter.drawLine(m_playheadX, m_drawBounds.top(), m_playheadX, m_drawBounds.bottom());
            }
            QColor handleColor = m_playheadStyle.handleColor;
            painter.setBrush(handleColor);
            painter.setPen(Qt::NoPen);
            QPolygon handlePoly;
            int halfW = 6;
            int bottomY = m_drawBounds.top();
            int topY = bottomY - HANDLE_HEIGHT;
            int midY = topY + 10; 
            handlePoly << QPoint(m_playheadX - halfW, topY);
            handlePoly << QPoint(m_playheadX + halfW, topY);
            handlePoly << QPoint(m_playheadX + halfW, midY);
            handlePoly << QPoint(m_playheadX, bottomY);
            handlePoly << QPoint(m_playheadX - halfW, midY);
            painter.drawPolygon(handlePoly);
        }
    }
    if (m_isCursorVisible && m_linesEnabled && !m_drawBounds.isEmpty()) {
        bool isCurrentPosValid = m_drawBounds.contains(m_currentPos);
        if (isCurrentPosValid) {
            const int x = m_currentPos.x();
            const int y = m_currentPos.y();
            QPen pen(m_style.color);
            pen.setWidth(m_style.lineWidth);
            pen.setCapStyle(Qt::FlatCap);
            painter.setPen(pen);
            int halfLen = m_style.lineLength / 2;
            int x1 = std::max(m_drawBounds.left(), x - halfLen);
            int x2 = std::min(m_drawBounds.right(), x + halfLen);
            int y1 = std::max(m_drawBounds.top(), y - halfLen);
            int y2 = std::min(m_drawBounds.bottom(), y + halfLen);
            painter.drawLine(x1, y, x2, y);
            painter.drawLine(x, y1, x, y2);
        }
    }
    if (m_isCursorVisible && !m_drawBounds.isEmpty() && m_drawBounds.contains(m_currentPos)) {
        m_indicator.draw(painter, m_currentPos, m_drawBounds);
    }
}
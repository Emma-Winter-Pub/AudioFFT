#include "CrosshairIndicator.h"

#include <QPainter>
#include <QFontMetrics>
#include <algorithm>
#include <QApplication>

CrosshairIndicator::CrosshairIndicator(){
    m_font = QApplication::font();
    m_font.setPointSize(8);
    m_textColor = QColor(230, 230, 230);
    m_bgColor = QColor(40, 45, 55, 150);
    m_borderColor = QColor(180, 180, 180);
}

void CrosshairIndicator::setData(const QString& freqText, const QString& timeText, const QString& dbText){
    m_freqText = freqText;
    m_timeText = timeText;
    m_dbText = dbText;
}

void CrosshairIndicator::setVisibility(bool showFreq, bool showTime, bool showDb){
    m_showFreq = showFreq;
    m_showTime = showTime;
    m_showDb = showDb;
}

void CrosshairIndicator::setStyle(const QFont& font, const QColor& textColor, const QColor& bgColor, const QColor& borderColor){
    m_font = font;
    m_textColor = textColor;
    m_bgColor = bgColor;
    m_borderColor = borderColor;
}

void CrosshairIndicator::draw(QPainter& painter, const QPoint& mousePos, const QRect& bounds){
    struct LineItem {
        QString text;
        QString label;
    };
    std::vector<QString> lines;
    if (m_showFreq && !m_freqText.isEmpty()) lines.push_back(m_freqText);
    if (m_showDb && !m_dbText.isEmpty())     lines.push_back(m_dbText);
    if (m_showTime && !m_timeText.isEmpty()) lines.push_back(m_timeText);
    if (lines.empty()) return;
    painter.setFont(m_font);
    QFontMetrics fm(m_font);
    int maxWidth = 0;
    int totalHeight = 0;
    int lineHeight = fm.height();
    for (const QString& text : lines) {
        int w = fm.horizontalAdvance(text);
        if (w > maxWidth) maxWidth = w;
        totalHeight += lineHeight;
    }
    if (lines.size() > 1) {
        totalHeight += (lines.size() - 1) * m_lineSpacing;
    }
    int boxWidth = maxWidth + m_padding * 2;
    int boxHeight = totalHeight + m_padding * 2;
    int x = mousePos.x() + m_cursorOffset;
    int y = mousePos.y() - boxHeight - m_cursorOffset;
    if (x + boxWidth > bounds.right()) {
        x = mousePos.x() - boxWidth - m_cursorOffset;
    }
    if (y < bounds.top()) {
        y = mousePos.y() + m_cursorOffset;
    }
    if (x < bounds.left()) x = bounds.left();
    if (y + boxHeight > bounds.bottom()) y = bounds.bottom() - boxHeight;
    QRect drawRect(x, y, boxWidth, boxHeight);
    painter.save();
    painter.setPen(QPen(m_borderColor, 1));
    painter.setBrush(m_bgColor);
    painter.drawRoundedRect(drawRect, 2, 2);
    painter.setPen(m_textColor);
    int currentY = drawRect.top() + m_padding;
    int ascent = fm.ascent(); 
    for (const QString& text : lines) {
        painter.drawText(drawRect.left() + m_padding, currentY + ascent, text);
        currentY += lineHeight + m_lineSpacing;
    }
    painter.restore();
}
#pragma once

#include <QString>
#include <QColor>
#include <QFont>
#include <QPoint>
#include <QRect>

class QPainter;

class CrosshairIndicator {
public:
    CrosshairIndicator();
    void setData(const QString& freqText, const QString& timeText, const QString& dbText);
    void setVisibility(bool showFreq, bool showTime, bool showDb);
    void setStyle(const QFont& font, const QColor& textColor, const QColor& bgColor, const QColor& borderColor);
    void draw(QPainter& painter, const QPoint& mousePos, const QRect& bounds);

private:
    QString m_freqText;
    QString m_timeText;
    QString m_dbText;
    bool m_showFreq = true;
    bool m_showTime = true;
    bool m_showDb = true;
    QFont m_font;
    QColor m_textColor;
    QColor m_bgColor;
    QColor m_borderColor;
    int m_padding = 6;
    int m_lineSpacing = 2;
    int m_cursorOffset = 15;
};
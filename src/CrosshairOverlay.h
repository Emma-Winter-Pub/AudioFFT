#pragma once

#include "CrosshairIndicator.h"

#include <QWidget>
#include <QColor>
#include <QPoint>
#include <QRect>

struct CrosshairStyle {
    int lineLength;
    int lineWidth;
    QColor color;
    CrosshairStyle()
        : lineLength(10000)
        , lineWidth(1)
        , color(Qt::white)
    { }
};

struct PlayheadStyle {
    bool visible;
    int lineWidth;
    QColor lineColor;
    QColor handleColor;
    PlayheadStyle()
        : visible(true)
        , lineWidth(1)
        , lineColor(45, 140, 235)
        , handleColor(45, 140, 235)
    { }
};

class CrosshairOverlay : public QWidget {
    Q_OBJECT

public:
    explicit CrosshairOverlay(QWidget *parent = nullptr);
    ~CrosshairOverlay() override;
    void updatePosition(const QPoint& pos);
    void setStyle(const CrosshairStyle& style);
    void setPlayheadStyle(const PlayheadStyle& style);
    void setCursorVisible(bool visible);
    void setDrawBounds(const QRect& bounds);
    bool isCursorVisible() const { return m_isCursorVisible; }
    void setPlayheadPosition(int x, bool visible);
    void setIndicatorData(const QString& freq, const QString& time, const QString& db);
    void setIndicatorVisibility(bool showFreq, bool showTime, bool showDb);
    void setLinesEnabled(bool enabled);

protected:
    void paintEvent(QPaintEvent *event) override;

private:
    CrosshairStyle m_style;
    PlayheadStyle m_playheadStyle;
    QPoint m_currentPos;
    bool m_isCursorVisible;
    QRect m_drawBounds;
    bool m_isPlayheadActive = false;
    int m_playheadX = 0;
    const int HANDLE_HEIGHT = 16;
    CrosshairIndicator m_indicator;
    bool m_linesEnabled = true;
};
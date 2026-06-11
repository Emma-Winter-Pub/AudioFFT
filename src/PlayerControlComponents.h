#pragma once

#include <QAbstractButton>
#include <QWidget>

enum class SymbolType {
    Play,
    Pause,
    Stop
};

class SymbolButton : public QAbstractButton {
    Q_OBJECT

public:
    explicit SymbolButton(QWidget* parent = nullptr);
    void setSymbolType(SymbolType type);
    SymbolType symbolType() const { return m_type; }
    void setBaseColor(const QColor& color);
    QSize sizeHint() const override;

protected:
    void paintEvent(QPaintEvent* event) override;
    void enterEvent(QEnterEvent* event) override;
    void leaveEvent(QEvent* event) override;

private:
    SymbolType m_type = SymbolType::Play;
    QColor m_baseColor;
    bool m_isHovered = false;
    void drawPlay(QPainter& p, const QRect& r, const QColor& c);
    void drawPause(QPainter& p, const QRect& r, const QColor& c);
    void drawStop(QPainter& p, const QRect& r, const QColor& c);
};
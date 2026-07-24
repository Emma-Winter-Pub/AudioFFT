#include "RibbonButton.h"

#include <QSizePolicy>

RibbonButton::RibbonButton(const QString& text, QWidget* parent)
    : QToolButton(parent)
{
    setText(text);
    setToolButtonStyle(Qt::ToolButtonTextOnly);
    setPopupMode(QToolButton::InstantPopup);
    setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);
    setFixedHeight(22);
}
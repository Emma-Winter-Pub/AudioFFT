#pragma once

#include <QToolButton>

class RibbonButton : public QToolButton
{
    Q_OBJECT

public:
    explicit RibbonButton(const QString& text, QWidget* parent = nullptr);
};
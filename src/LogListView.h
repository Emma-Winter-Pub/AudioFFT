#pragma once

#include <QListView>

class LogListView : public QListView {
    Q_OBJECT
public:
    explicit LogListView(QWidget* parent = nullptr);

protected:
    void keyPressEvent(class QKeyEvent* event) override;

private:
    bool m_autoScroll = true;
};
#include "LogListView.h"

#include <QScrollBar>
#include <QKeyEvent>
#include <QApplication>
#include <QClipboard>
#include <algorithm>

LogListView::LogListView(QWidget* parent) : QListView(parent) {
    setUniformItemSizes(true);
    setSelectionMode(QAbstractItemView::ExtendedSelection);
    setEditTriggers(QAbstractItemView::NoEditTriggers);
    setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);
    connect(verticalScrollBar(), &QScrollBar::valueChanged, this, [this](int value) {
        m_autoScroll = (value >= verticalScrollBar()->maximum() - 5);
    });
    connect(verticalScrollBar(), &QScrollBar::rangeChanged, this, [this](int min, int max) {
        Q_UNUSED(min);
        if (m_autoScroll) {
            verticalScrollBar()->setValue(max);
        }
    });
}

void LogListView::keyPressEvent(QKeyEvent* event) {
    if (event->matches(QKeySequence::Copy)) {
        QModelIndexList selected = selectionModel()->selectedIndexes();
        if (selected.isEmpty()) return;
        std::sort(selected.begin(), selected.end());
        QString text;
        for (const QModelIndex& idx : selected) {
            text += idx.data(Qt::DisplayRole).toString() + "\n";
        }
        QApplication::clipboard()->setText(text);
        event->accept();
    } else if (event->key() == Qt::Key_Space) {
        event->ignore();
    } else {
        QListView::keyPressEvent(event);
    }
}
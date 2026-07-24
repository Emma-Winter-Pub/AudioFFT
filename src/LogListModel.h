#pragma once

#include <QAbstractListModel>
#include <QStringList>
#include <QTimer>
#include <vector>

class LogListModel : public QAbstractListModel {
    Q_OBJECT
public:
    explicit LogListModel(QObject* parent = nullptr);
    int rowCount(const QModelIndex& parent = QModelIndex()) const override;
    QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;

public slots:
    void appendLog(const QString& message);
    void clear();

private slots:
    void flushBuffer();

private:
    std::vector<QString> m_logs;
    std::vector<QString> m_buffer;
    QTimer m_timer;
};
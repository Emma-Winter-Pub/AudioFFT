#include "LogListModel.h"

LogListModel::LogListModel(QObject* parent) : QAbstractListModel(parent) {
    m_logs.reserve(200000);
    m_buffer.reserve(5000);
    connect(&m_timer, &QTimer::timeout, this, &LogListModel::flushBuffer);
}

int LogListModel::rowCount(const QModelIndex& parent) const {
    if (parent.isValid()) return 0;
    return static_cast<int>(m_logs.size());
}

QVariant LogListModel::data(const QModelIndex& index, int role) const {
    if (!index.isValid() || index.row() >= static_cast<int>(m_logs.size())) {
        return QVariant();
    }
    if (role == Qt::DisplayRole || role == Qt::EditRole) {
        return m_logs[index.row()];
    }
    return QVariant();
}

void LogListModel::appendLog(const QString& message) {
    QStringList lines = message.split('\n');
    if (!lines.isEmpty() && lines.last().isEmpty()) {
        lines.removeLast();
    }
    m_buffer.insert(m_buffer.end(), lines.begin(), lines.end());
    if (!m_timer.isActive()) {
        m_timer.start(50);
    }
}

void LogListModel::clear() {
    beginResetModel();
    m_logs.clear();
    m_buffer.clear();
    endResetModel();
}

void LogListModel::flushBuffer() {
    if (m_buffer.empty()) {
        m_timer.stop();
        return;
    }
    int start = static_cast<int>(m_logs.size());
    int count = static_cast<int>(m_buffer.size());
    beginInsertRows(QModelIndex(), start, start + count - 1);
    m_logs.insert(m_logs.end(), m_buffer.begin(), m_buffer.end());
    m_buffer.clear();
    endInsertRows();
    m_timer.stop();
}
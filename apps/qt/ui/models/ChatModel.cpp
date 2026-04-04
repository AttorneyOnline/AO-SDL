#include "ChatModel.h"

ChatModel::ChatModel(QObject* parent) : QAbstractListModel(parent) {
}

void ChatModel::appendLine(const ChatLine& line) {
    const int row = static_cast<int>(m_lines.size());
    beginInsertRows({}, row, row);
    m_lines.push_back(line);
    endInsertRows();
}

void ChatModel::clear() {
    if (m_lines.empty())
        return;
    beginResetModel();
    m_lines.clear();
    endResetModel();
}

int ChatModel::rowCount(const QModelIndex& parent) const {
    if (parent.isValid())
        return 0;
    return static_cast<int>(m_lines.size());
}

QVariant ChatModel::data(const QModelIndex& index, int role) const {
    if (!index.isValid() || index.row() < 0 || index.row() >= static_cast<int>(m_lines.size()))
        return {};

    const ChatLine& l = m_lines[index.row()];
    switch (role) {
    case SenderRole:
        return l.sender;
    case MessageRole:
        return l.message;
    case IsSystemRole:
        return l.isSystem;
    default:
        return {};
    }
}

QHash<int, QByteArray> ChatModel::roleNames() const {
    return {
        {SenderRole, "sender"},
        {MessageRole, "message"},
        {IsSystemRole, "isSystem"},
    };
}

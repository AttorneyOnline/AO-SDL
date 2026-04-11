#include "ICLogModel.h"

ICLogModel::ICLogModel(QObject* parent) : QAbstractListModel(parent) {
}

void ICLogModel::appendEntry(const Entry& entry) {
    // Drop oldest entries once the cap is reached.
    if (static_cast<int>(m_entries.size()) >= kMaxEntries) {
        beginRemoveRows({}, 0, 0);
        m_entries.erase(m_entries.begin());
        endRemoveRows();
    }
    beginInsertRows({}, static_cast<int>(m_entries.size()), static_cast<int>(m_entries.size()));
    m_entries.push_back(entry);
    endInsertRows();
}

void ICLogModel::clear() {
    if (m_entries.empty())
        return;
    beginResetModel();
    m_entries.clear();
    endResetModel();
}

int ICLogModel::rowCount(const QModelIndex& parent) const {
    return parent.isValid() ? 0 : static_cast<int>(m_entries.size());
}

QVariant ICLogModel::data(const QModelIndex& index, int role) const {
    if (!index.isValid() || index.row() < 0 || index.row() >= rowCount())
        return {};
    const auto& e = m_entries[index.row()];
    switch (role) {
    case ShownameRole:
        return e.showname;
    case MessageRole:
        return e.message;
    case ColorIdxRole:
        return e.colorIdx;
    default:
        return {};
    }
}

QHash<int, QByteArray> ICLogModel::roleNames() const {
    return {
        {ShownameRole, "showname"},
        {MessageRole, "message"},
        {ColorIdxRole, "colorIdx"},
    };
}

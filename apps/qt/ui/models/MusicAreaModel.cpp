#include "MusicAreaModel.h"

MusicAreaModel::MusicAreaModel(QObject* parent) : QAbstractListModel(parent) {
}

void MusicAreaModel::reset(const std::vector<Entry>& entries) {
    beginResetModel();
    m_entries = entries;
    endResetModel();
}

void MusicAreaModel::clear() {
    if (m_entries.empty())
        return;
    beginResetModel();
    m_entries.clear();
    endResetModel();
}

int MusicAreaModel::rowCount(const QModelIndex& parent) const {
    if (parent.isValid())
        return 0;
    return static_cast<int>(m_entries.size());
}

QVariant MusicAreaModel::data(const QModelIndex& index, int role) const {
    if (!index.isValid() || index.row() < 0 || index.row() >= static_cast<int>(m_entries.size()))
        return {};

    const Entry& e = m_entries[index.row()];
    switch (role) {
    case NameRole:
        return e.name;
    case PlayerCountRole:
        return e.playerCount;
    case IsAreaRole:
        return e.isArea;
    case StatusRole:
        return e.status;
    case CmRole:
        return e.cm;
    case LockRole:
        return e.lock;
    default:
        return {};
    }
}

QHash<int, QByteArray> MusicAreaModel::roleNames() const {
    return {
        {NameRole, "name"},     {PlayerCountRole, "playerCount"},
        {IsAreaRole, "isArea"}, {StatusRole, "status"},
        {CmRole, "cm"},         {LockRole, "lock"},
    };
}

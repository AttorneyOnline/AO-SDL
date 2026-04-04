#include "CharListModel.h"

CharListModel::CharListModel(QObject* parent) : QAbstractListModel(parent) {
}

void CharListModel::clear() {
    if (m_entries.empty())
        return;
    beginResetModel();
    m_entries.clear();
    endResetModel();
}

void CharListModel::appendBatch(const std::vector<CharEntry>& batch) {
    if (batch.empty())
        return;
    int first = static_cast<int>(m_entries.size());
    int last = first + static_cast<int>(batch.size()) - 1;
    beginInsertRows({}, first, last);
    m_entries.insert(m_entries.end(), batch.begin(), batch.end());
    endInsertRows();
}

void CharListModel::setTaken(int row, bool taken) {
    if (row < 0 || row >= static_cast<int>(m_entries.size()))
        return;
    if (m_entries[row].taken == taken)
        return;
    m_entries[row].taken = taken;
    QModelIndex idx = index(row);
    emit dataChanged(idx, idx, {TakenRole});
}

void CharListModel::setIconSource(int row, const QString& source) {
    if (row < 0 || row >= static_cast<int>(m_entries.size()))
        return;
    if (m_entries[row].iconSource == source)
        return;
    m_entries[row].iconSource = source;
    QModelIndex idx = index(row);
    emit dataChanged(idx, idx, {IconSourceRole});
}

int CharListModel::rowCount(const QModelIndex& parent) const {
    if (parent.isValid())
        return 0;
    return static_cast<int>(m_entries.size());
}

QVariant CharListModel::data(const QModelIndex& index, int role) const {
    if (!index.isValid() || index.row() < 0 || index.row() >= static_cast<int>(m_entries.size()))
        return {};

    const CharEntry& e = m_entries[index.row()];
    switch (role) {
    case NameRole:
        return e.name;
    case TakenRole:
        return e.taken;
    case IconSourceRole:
        return e.iconSource;
    default:
        return {};
    }
}

QHash<int, QByteArray> CharListModel::roleNames() const {
    return {
        {NameRole, "name"},
        {TakenRole, "taken"},
        {IconSourceRole, "iconSource"},
    };
}

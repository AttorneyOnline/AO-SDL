#include "CharListModel.h"

CharListModel::CharListModel(QObject* parent)
    : QAbstractListModel(parent) {}

void CharListModel::reset(const std::vector<CharEntry>& chars) {
    beginResetModel();
    m_entries = chars;
    endResetModel();
}

int CharListModel::rowCount(const QModelIndex& parent) const {
    if (parent.isValid())
        return 0;
    return static_cast<int>(m_entries.size());
}

QVariant CharListModel::data(const QModelIndex& index, int role) const {
    if (!index.isValid() || index.row() < 0
        || index.row() >= static_cast<int>(m_entries.size()))
        return {};

    const CharEntry& e = m_entries[index.row()];
    switch (role) {
    case NameRole:  return e.name;
    case TakenRole: return e.taken;
    default:        return {};
    }
}

QHash<int, QByteArray> CharListModel::roleNames() const {
    return {
        { NameRole,  "name"  },
        { TakenRole, "taken" },
    };
}

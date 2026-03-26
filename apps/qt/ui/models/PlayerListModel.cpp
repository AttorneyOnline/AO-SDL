#include "PlayerListModel.h"

PlayerListModel::PlayerListModel(QObject* parent)
    : QAbstractListModel(parent) {}

void PlayerListModel::reset(const std::map<int, PlayerEntry>& players) {
    beginResetModel();
    m_entries.clear();
    m_entries.reserve(players.size());
    for (const auto& [id, p] : players)
        m_entries.push_back(p);
    endResetModel();
}

void PlayerListModel::clear() {
    if (m_entries.empty())
        return;
    beginResetModel();
    m_entries.clear();
    endResetModel();
}

int PlayerListModel::rowCount(const QModelIndex& parent) const {
    if (parent.isValid())
        return 0;
    return static_cast<int>(m_entries.size());
}

QVariant PlayerListModel::data(const QModelIndex& index, int role) const {
    if (!index.isValid() || index.row() < 0
        || index.row() >= static_cast<int>(m_entries.size()))
        return {};

    const PlayerEntry& p = m_entries[index.row()];
    switch (role) {
    case PlayerIdRole:  return p.playerId;
    case NameRole:      return p.name;
    case CharacterRole: return p.character;
    case AreaIdRole:    return p.areaId;
    default:            return {};
    }
}

QHash<int, QByteArray> PlayerListModel::roleNames() const {
    return {
        { PlayerIdRole,  "playerId"  },
        { NameRole,      "name"      },
        { CharacterRole, "character" },
        { AreaIdRole,    "areaId"    },
    };
}

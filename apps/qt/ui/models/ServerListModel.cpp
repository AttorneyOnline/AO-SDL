#include "ServerListModel.h"

ServerListModel::ServerListModel(QObject* parent) : QAbstractListModel(parent) {
}

void ServerListModel::reset(const std::vector<ServerEntry>& servers) {
    beginResetModel();
    m_entries = servers;
    endResetModel();
}

int ServerListModel::rowCount(const QModelIndex& parent) const {
    if (parent.isValid())
        return 0;
    return static_cast<int>(m_entries.size());
}

QVariant ServerListModel::data(const QModelIndex& index, int role) const {
    if (!index.isValid() || index.row() < 0 || index.row() >= static_cast<int>(m_entries.size()))
        return {};

    const ServerEntry& e = m_entries[index.row()];
    switch (role) {
    case NameRole:
        return QString::fromStdString(e.name);
    case DescriptionRole:
        return QString::fromStdString(e.description);
    case PlayersRole:
        return e.players;
    case HostnameRole:
        return QString::fromStdString(e.hostname);
    default:
        return {};
    }
}

QHash<int, QByteArray> ServerListModel::roleNames() const {
    return {
        {NameRole, "name"},
        {DescriptionRole, "description"},
        {PlayersRole, "players"},
        {HostnameRole, "hostname"},
    };
}

#pragma once

#include "game/ServerList.h"

#include <QAbstractListModel>
#include <QString>

#include <vector>

/**
 * @brief QML model backed by the master-server list.
 *
 * Populated by ServerListController::sync() when a ServerListEvent arrives
 * or when the screen transitions in.  Roles: name, description, players,
 * hostname.
 */
class ServerListModel : public QAbstractListModel {
    Q_OBJECT

  public:
    enum Role {
        NameRole        = Qt::UserRole + 1,
        DescriptionRole,
        PlayersRole,
        HostnameRole,
    };

    explicit ServerListModel(QObject* parent = nullptr);

    /// Replace all rows with the given server list.
    void reset(const std::vector<ServerEntry>& servers);

    int rowCount(const QModelIndex& parent = {}) const override;
    QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;
    QHash<int, QByteArray> roleNames() const override;

  private:
    std::vector<ServerEntry> m_entries;
};

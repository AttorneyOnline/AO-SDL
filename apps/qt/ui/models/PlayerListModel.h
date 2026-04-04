#pragma once

#include <QAbstractListModel>
#include <QString>

#include <map>
#include <string>

/**
 * @brief QML model for the connected player list.
 *
 * Populated by CourtroomController when PR/PU events arrive.
 * Roles: playerId, name, character, areaId.
 */
class PlayerListModel : public QAbstractListModel {
    Q_OBJECT

  public:
    enum Role {
        PlayerIdRole = Qt::UserRole + 1,
        NameRole,
        CharacterRole,
        AreaIdRole,
    };

    struct PlayerEntry {
        int playerId = -1;
        QString name;
        QString character;
        int areaId = -1;
    };

    explicit PlayerListModel(QObject* parent = nullptr);

    /// Replace the full player list (keyed by player ID).
    void reset(const std::map<int, PlayerEntry>& players);

    /// Clear all entries (e.g. on disconnect).
    void clear();

    int rowCount(const QModelIndex& parent = {}) const override;
    QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;
    QHash<int, QByteArray> roleNames() const override;

  private:
    std::vector<PlayerEntry> m_entries;
};

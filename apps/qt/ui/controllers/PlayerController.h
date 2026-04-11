#pragma once

#include "IQtScreenController.h"
#include "ui/models/PlayerListModel.h"

#include <QObject>

#include <map>
#include <string>

/**
 * @brief Qt controller for the connected player roster.
 *
 * drain() consumes PlayerListEvent and maintains an incremental player cache,
 * rebuilding PlayerListModel only when the roster changes.
 */
class PlayerController : public IQtScreenController {
    Q_OBJECT
    Q_PROPERTY(PlayerListModel* playerListModel READ playerListModel CONSTANT)

  public:
    explicit PlayerController(QObject* parent = nullptr);

    void drain() override;

    PlayerListModel* playerListModel() {
        return &m_players;
    }

    void reset();

  private:
    struct PlayerCacheEntry {
        QString name;
        QString character;
        int areaId = -1;
    };

    void rebuildPlayerModel();

    PlayerListModel m_players;
    std::map<int, PlayerCacheEntry> m_playerCache;
};

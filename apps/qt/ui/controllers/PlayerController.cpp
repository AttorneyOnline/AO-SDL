#include "PlayerController.h"

#include "event/EventManager.h"
#include "event/PlayerListEvent.h"

#include <cstdlib>

PlayerController::PlayerController(QObject* parent) : IQtScreenController(parent) {
}

void PlayerController::drain() {
    auto& ch = EventManager::instance().get_channel<PlayerListEvent>();
    bool dirty = false;
    while (auto ev = ch.get_event()) {
        dirty = true;
        int id = ev->player_id();
        switch (ev->action()) {
        case PlayerListEvent::Action::ADD:
            m_playerCache[id] = {QString::fromStdString(ev->data()), {}, -1};
            break;
        case PlayerListEvent::Action::REMOVE:
            m_playerCache.erase(id);
            break;
        case PlayerListEvent::Action::UPDATE_NAME:
            m_playerCache[id].name = QString::fromStdString(ev->data());
            break;
        case PlayerListEvent::Action::UPDATE_CHARACTER:
            m_playerCache[id].character = QString::fromStdString(ev->data());
            break;
        case PlayerListEvent::Action::UPDATE_CHARNAME:
            m_playerCache[id].name = QString::fromStdString(ev->data());
            break;
        case PlayerListEvent::Action::UPDATE_AREA:
            m_playerCache[id].areaId = std::atoi(ev->data().c_str());
            break;
        }
    }
    if (dirty)
        rebuildPlayerModel();
}

void PlayerController::reset() {
    m_playerCache.clear();
    m_players.clear();
}

void PlayerController::rebuildPlayerModel() {
    std::map<int, PlayerListModel::PlayerEntry> entries;
    for (const auto& [id, c] : m_playerCache)
        entries[id] = {id, c.name, c.character, c.areaId};
    m_players.reset(entries);
}

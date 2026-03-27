#include "CourtroomController.h"

#include "event/AreaUpdateEvent.h"
#include "event/ChatEvent.h"
#include "event/EvidenceListEvent.h"
#include "event/EventManager.h"
#include "event/HealthBarEvent.h"
#include "event/MusicListEvent.h"
#include "event/NowPlayingEvent.h"
#include "event/PlayerListEvent.h"
#include "ui/UIManager.h"

#include <algorithm>
#include <cstdlib>

CourtroomController::CourtroomController(UIManager& uiMgr, QObject* parent)
    : IQtScreenController(parent)
    , m_uiMgr(uiMgr)
{}

void CourtroomController::setInitialCharName(const std::string& name) {
    QString qname = QString::fromStdString(name);
    if (qname == m_charName)
        return;
    m_charName = qname;
    emit charNameChanged();
}

void CourtroomController::drain() {
    drainChat();
    drainPlayerList();
    drainEvidence();
    drainMusicList();
    drainAreaUpdates();
    drainHealthBars();
    drainNowPlaying();
}

void CourtroomController::reset() {
    m_chat.clear();
    m_players.clear();
    m_evidence.clear();
    m_musicArea.clear();
    m_areas.clear();
    m_tracks.clear();
    m_playerCache.clear();

    if (m_defHp != 0) { m_defHp = 0; emit defHpChanged(); }
    if (m_proHp != 0) { m_proHp = 0; emit proHpChanged(); }
    if (!m_nowPlaying.isEmpty()) { m_nowPlaying.clear(); emit nowPlayingChanged(); }
    if (!m_charName.isEmpty())   { m_charName.clear();   emit charNameChanged();  }
}

void CourtroomController::disconnect() {
    reset();
    m_uiMgr.pop_to_root();
}

// --------------------------------------------------------------------------
// Event drains
// --------------------------------------------------------------------------

void CourtroomController::drainChat() {
    auto& ch = EventManager::instance().get_channel<ChatEvent>();
    while (auto ev = ch.get_event()) {
        m_chat.appendLine({
            QString::fromStdString(ev->get_sender_name()),
            QString::fromStdString(ev->get_message()),
            ev->get_system_message()
        });
    }
}

void CourtroomController::drainPlayerList() {
    auto& ch = EventManager::instance().get_channel<PlayerListEvent>();
    bool dirty = false;
    while (auto ev = ch.get_event()) {
        dirty = true;
        int id = ev->player_id();
        switch (ev->action()) {
        case PlayerListEvent::Action::ADD:
            m_playerCache[id] = { QString::fromStdString(ev->data()), {}, -1 };
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
            // charname (display name) replaces the stored name
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

void CourtroomController::drainEvidence() {
    auto& ch = EventManager::instance().get_channel<EvidenceListEvent>();
    while (auto ev = ch.get_event())
        m_evidence.reset(ev->items());
}

void CourtroomController::drainMusicList() {
    auto& ch = EventManager::instance().get_channel<MusicListEvent>();
    bool dirty = false;
    while (auto ev = ch.get_event()) {
        dirty = true;
        if (ev->partial()) {
            if (!ev->areas().empty()) {
                m_areas.clear();
                m_areas.resize(ev->areas().size());
                for (size_t i = 0; i < ev->areas().size(); i++)
                    m_areas[i].name = ev->areas()[i];
            }
            if (!ev->tracks().empty())
                m_tracks = ev->tracks();
        } else {
            m_areas.clear();
            m_areas.resize(ev->areas().size());
            for (size_t i = 0; i < ev->areas().size(); i++)
                m_areas[i].name = ev->areas()[i];
            m_tracks = ev->tracks();
        }
    }
    if (dirty)
        rebuildMusicAreaModel();
}

void CourtroomController::drainAreaUpdates() {
    auto& ch = EventManager::instance().get_channel<AreaUpdateEvent>();
    bool dirty = false;
    while (auto ev = ch.get_event()) {
        dirty = true;
        const auto& vals = ev->values();
        size_t count = std::min(vals.size(), m_areas.size());
        for (size_t i = 0; i < count; i++) {
            switch (ev->type()) {
            case AreaUpdateEvent::PLAYERS:
                m_areas[i].playerCount = std::atoi(vals[i].c_str());
                break;
            case AreaUpdateEvent::STATUS:
                m_areas[i].status = vals[i];
                break;
            case AreaUpdateEvent::CM:
                m_areas[i].cm = vals[i];
                break;
            case AreaUpdateEvent::LOCK:
                m_areas[i].lock = vals[i];
                break;
            }
        }
    }
    if (dirty)
        rebuildMusicAreaModel();
}

void CourtroomController::drainHealthBars() {
    auto& ch = EventManager::instance().get_channel<HealthBarEvent>();
    while (auto ev = ch.get_event()) {
        if (ev->side() == 1 && ev->value() != m_defHp) {
            m_defHp = ev->value();
            emit defHpChanged();
        } else if (ev->side() == 2 && ev->value() != m_proHp) {
            m_proHp = ev->value();
            emit proHpChanged();
        }
    }
}

void CourtroomController::drainNowPlaying() {
    auto& ch = EventManager::instance().get_channel<NowPlayingEvent>();
    while (auto ev = ch.get_event()) {
        QString np = QString::fromStdString(ev->track());
        if (np != m_nowPlaying) {
            m_nowPlaying = np;
            emit nowPlayingChanged();
        }
    }
}

// --------------------------------------------------------------------------
// Model rebuilds
// --------------------------------------------------------------------------

void CourtroomController::rebuildMusicAreaModel() {
    std::vector<MusicAreaModel::Entry> entries;
    entries.reserve(m_areas.size() + m_tracks.size());

    for (const auto& a : m_areas) {
        MusicAreaModel::Entry e;
        e.name        = QString::fromStdString(a.name);
        e.playerCount = a.playerCount;
        e.isArea      = true;
        e.status      = QString::fromStdString(a.status);
        e.cm          = QString::fromStdString(a.cm);
        e.lock        = QString::fromStdString(a.lock);
        entries.push_back(std::move(e));
    }
    for (const auto& t : m_tracks) {
        MusicAreaModel::Entry e;
        e.name   = QString::fromStdString(t);
        e.isArea = false;
        entries.push_back(std::move(e));
    }

    m_musicArea.reset(entries);
}

void CourtroomController::rebuildPlayerModel() {
    std::map<int, PlayerListModel::PlayerEntry> entries;
    for (const auto& [id, c] : m_playerCache) {
        entries[id] = { id, c.name, c.character, c.areaId };
    }
    m_players.reset(entries);
}

#include "CourtroomController.h"

#include "ao/ui/screens/CourtroomScreen.h"
#include "event/AreaUpdateEvent.h"
#include "event/ChatEvent.h"
#include "event/EventManager.h"
#include "event/EvidenceListEvent.h"
#include "event/HealthBarEvent.h"
#include "event/MusicListEvent.h"
#include "event/NowPlayingEvent.h"
#include "event/OutgoingICMessageEvent.h"
#include "event/PlayerListEvent.h"
#include "ui/UIManager.h"

#include "utils/Log.h"

#include <algorithm>
#include <cstdlib>

CourtroomController::CourtroomController(UIManager& uiMgr, QObject* parent)
    : IQtScreenController(parent), m_uiMgr(uiMgr) {
}

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
    applyCharacterData();
}

void CourtroomController::reset() {
    Log::debug("[CourtroomController] reset");
    m_chat.clear();
    m_players.clear();
    m_evidence.clear();
    m_musicArea.clear();
    m_emotes.clear();
    m_areas.clear();
    m_tracks.clear();
    m_playerCache.clear();

    if (m_defHp != 0) {
        m_defHp = 0;
        emit defHpChanged();
    }
    if (m_proHp != 0) {
        m_proHp = 0;
        emit proHpChanged();
    }
    if (!m_nowPlaying.isEmpty()) {
        m_nowPlaying.clear();
        emit nowPlayingChanged();
    }
    if (!m_charName.isEmpty()) {
        m_charName.clear();
        emit charNameChanged();
    }
    if (!m_showname.isEmpty()) {
        m_showname.clear();
        emit shownameChanged();
    }
    if (!m_side.isEmpty()) {
        m_side.clear();
        emit sideChanged();
    }
    if (m_selectedEmote != 0) {
        m_selectedEmote = 0;
        emit selectedEmoteChanged();
    }
    if (m_preAnim) {
        m_preAnim = false;
        emit preAnimChanged();
    }
    m_charId = -1;
    m_lastLoadGen = -1;
}

void CourtroomController::disconnect() {
    Log::info("[CourtroomController] disconnecting, returning to server list");
    reset();
    m_uiMgr.pop_to_root();
}

void CourtroomController::setShowname(const QString& v) {
    if (v == m_showname)
        return;
    m_showname = v;
    emit shownameChanged();
}

void CourtroomController::setPreAnim(bool v) {
    if (v == m_preAnim)
        return;
    m_preAnim = v;
    emit preAnimChanged();
}

void CourtroomController::selectEmote(int index) {
    if (index < 0 || index >= m_emotes.rowCount())
        return;
    if (m_selectedEmote != index) {
        m_selectedEmote = index;
        emit selectedEmoteChanged();
    }

    // Auto-update pre-anim flag from the selected emote.
    auto* screen = dynamic_cast<CourtroomScreen*>(m_uiMgr.active_screen());
    if (!screen)
        return;
    auto sheet = screen->get_character_sheet();
    if (sheet && index < sheet->emote_count()) {
        const auto& emo = sheet->emote(index);
        bool hasPreAnim = !emo.pre_anim.empty() && emo.pre_anim != "-";
        setPreAnim(hasPreAnim);
    }
}

void CourtroomController::sendICMessage(const QString& message, int objectionMod) {
    ICMessageData data;
    data.character = m_charName.toStdString();
    data.char_id = m_charId;
    data.message = message.toStdString();
    data.showname = m_showname.toStdString();
    data.side = m_side.isEmpty() ? "def" : m_side.toStdString();
    data.objection_mod = objectionMod;

    auto* screen = dynamic_cast<CourtroomScreen*>(m_uiMgr.active_screen());
    if (screen) {
        auto sheet = screen->get_character_sheet();
        if (sheet && m_selectedEmote >= 0 && m_selectedEmote < sheet->emote_count()) {
            const auto& emo = sheet->emote(m_selectedEmote);
            data.emote = emo.anim_name;
            data.pre_emote = emo.pre_anim;
            data.desk_mod = emo.desk_mod;
            if (!emo.sfx_name.empty() && emo.sfx_name != "0") {
                data.sfx_name = emo.sfx_name;
                data.sfx_delay = emo.sfx_delay;
            }
        }
    }

    data.emote_mod = m_preAnim ? 1 : 0;

    EventManager::instance().get_channel<OutgoingICMessageEvent>().publish(OutgoingICMessageEvent(std::move(data)));
}

// --------------------------------------------------------------------------
// Character sheet polling
// --------------------------------------------------------------------------

void CourtroomController::applyCharacterData() {
    auto* screen = dynamic_cast<CourtroomScreen*>(m_uiMgr.active_screen());
    if (!screen || screen->is_loading())
        return;
    if (screen->load_generation() == m_lastLoadGen)
        return;

    m_lastLoadGen = screen->load_generation();
    m_charId = screen->get_char_id();

    QString qname = QString::fromStdString(screen->get_character_name());
    if (qname != m_charName) {
        m_charName = qname;
        emit charNameChanged();
    }

    auto sheet = screen->get_character_sheet();
    if (sheet) {
        QString qside = QString::fromStdString(sheet->side());
        if (qside != m_side) {
            m_side = qside;
            emit sideChanged();
        }

        QString qshow = QString::fromStdString(sheet->showname());
        if (qshow != m_showname) {
            m_showname = qshow;
            emit shownameChanged();
        }
    }

    // Build emote entries; icons are served on-demand by EmoteIconProvider.
    std::vector<EmoteModel::Entry> entries;
    int emoteCount = sheet ? sheet->emote_count() : 0;
    entries.reserve(emoteCount);
    for (int i = 0; i < emoteCount; i++) {
        EmoteModel::Entry e;
        e.comment = QString::fromStdString(sheet->emote(i).comment);
        e.iconSource = QStringLiteral("image://emoteicon/%1/%2").arg(qname).arg(i);
        entries.push_back(std::move(e));
    }
    m_emotes.reset(std::move(entries));

    // Reset selection to first emote and auto-set pre-anim.
    int newSel = 0;
    bool newPre = false;
    if (sheet && emoteCount > 0) {
        const auto& emo = sheet->emote(0);
        newPre = !emo.pre_anim.empty() && emo.pre_anim != "-";
    }
    if (m_selectedEmote != newSel) {
        m_selectedEmote = newSel;
        emit selectedEmoteChanged();
    }
    if (m_preAnim != newPre) {
        m_preAnim = newPre;
        emit preAnimChanged();
    }

    Log::debug("[CourtroomController] character data applied: {} ({} emotes)", screen->get_character_name(),
               emoteCount);
}

// --------------------------------------------------------------------------
// Event drains
// --------------------------------------------------------------------------

void CourtroomController::drainChat() {
    auto& ch = EventManager::instance().get_channel<ChatEvent>();
    while (auto ev = ch.get_event()) {
        m_chat.appendLine({QString::fromStdString(ev->get_sender_name()), QString::fromStdString(ev->get_message()),
                           ev->get_system_message()});
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
        }
        else {
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
        }
        else if (ev->side() == 2 && ev->value() != m_proHp) {
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
        e.name = QString::fromStdString(a.name);
        e.playerCount = a.playerCount;
        e.isArea = true;
        e.status = QString::fromStdString(a.status);
        e.cm = QString::fromStdString(a.cm);
        e.lock = QString::fromStdString(a.lock);
        entries.push_back(std::move(e));
    }
    for (const auto& t : m_tracks) {
        MusicAreaModel::Entry e;
        e.name = QString::fromStdString(t);
        e.isArea = false;
        entries.push_back(std::move(e));
    }

    m_musicArea.reset(entries);
}

void CourtroomController::rebuildPlayerModel() {
    std::map<int, PlayerListModel::PlayerEntry> entries;
    for (const auto& [id, c] : m_playerCache) {
        entries[id] = {id, c.name, c.character, c.areaId};
    }
    m_players.reset(entries);
}

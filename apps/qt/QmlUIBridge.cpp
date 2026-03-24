#include "QmlUIBridge.h"

#include "ao/ui/screens/CharSelectScreen.h"
#include "ao/ui/screens/CourtroomScreen.h"
#include "ao/ui/screens/ServerListScreen.h"
#include "ui/Screen.h"
#include "ui/UIManager.h"

#include "event/AreaUpdateEvent.h"
#include "event/ChatEvent.h"
#include "event/EventManager.h"
#include "event/EvidenceListEvent.h"
#include "event/HealthBarEvent.h"
#include "event/ICLogEvent.h"
#include "event/MusicListEvent.h"
#include "event/NowPlayingEvent.h"
#include "event/OutgoingChatEvent.h"
#include "event/OutgoingHealthBarEvent.h"
#include "event/OutgoingICMessageEvent.h"
#include "event/OutgoingMusicEvent.h"
#include "event/PlayerListEvent.h"
#include "event/TimerEvent.h"
#include "event/VolumeChangeEvent.h"
#include "game/ICharacterSheet.h"

#include <algorithm>

static constexpr const char* SIDES[] = {"def", "pro", "wit", "jud", "jur", "sea", "hlp"};

// ============================================================================
// ServerListModel
// ============================================================================

int ServerListModel::rowCount(const QModelIndex& parent) const {
    return parent.isValid() ? 0 : entries_.size();
}

QVariant ServerListModel::data(const QModelIndex& index, int role) const {
    if (!index.isValid() || index.row() >= entries_.size())
        return {};

    const auto& e = entries_[index.row()];
    switch (role) {
    case NameRole:
        return e.name;
    case PlayersRole:
        return e.players;
    case DescriptionRole:
        return e.description;
    case HasWsRole:
        return e.has_ws;
    default:
        return {};
    }
}

QHash<int, QByteArray> ServerListModel::roleNames() const {
    return {
        {NameRole, "name"},
        {PlayersRole, "players"},
        {DescriptionRole, "description"},
        {HasWsRole, "hasWs"},
    };
}

void ServerListModel::update_from(const std::vector<ServerEntry>& servers) {
    beginResetModel();
    entries_.clear();
    entries_.reserve(static_cast<int>(servers.size()));
    for (const auto& s : servers) {
        entries_.append({
            QString::fromStdString(s.name),
            s.players,
            QString::fromStdString(s.description),
            s.ws_port.has_value() || s.wss_port.has_value(),
        });
    }
    endResetModel();
}

// ============================================================================
// CharListModel
// ============================================================================

int CharListModel::rowCount(const QModelIndex& parent) const {
    return parent.isValid() ? 0 : entries_.size();
}

QVariant CharListModel::data(const QModelIndex& index, int role) const {
    if (!index.isValid() || index.row() >= entries_.size())
        return {};

    const auto& e = entries_[index.row()];
    switch (role) {
    case FolderRole:
        return e.folder;
    case HasIconRole:
        return e.has_icon;
    case TakenRole:
        return e.taken;
    default:
        return {};
    }
}

QHash<int, QByteArray> CharListModel::roleNames() const {
    return {
        {FolderRole, "folder"},
        {HasIconRole, "hasIcon"},
        {TakenRole, "taken"},
    };
}

void CharListModel::reset(int count, const std::function<QString(int)>& folder_fn) {
    beginResetModel();
    entries_.resize(count);
    for (int i = 0; i < count; ++i)
        entries_[i] = {folder_fn(i), false, false};
    endResetModel();
}

void CharListModel::refresh_status(int count, const std::function<CharSnapshot(int)>& snapshot_fn) {
    if (count != entries_.size())
        return;
    for (int i = 0; i < count; ++i) {
        auto snap = snapshot_fn(i);
        bool changed = false;
        if (entries_[i].taken != snap.taken) {
            entries_[i].taken = snap.taken;
            changed = true;
        }
        if (!entries_[i].has_icon && snap.has_icon) {
            entries_[i].has_icon = true;
            entries_[i].icon = std::move(snap.image);
            changed = true;
        }
        if (changed) {
            QModelIndex idx = index(i);
            emit dataChanged(idx, idx, {TakenRole, HasIconRole});
        }
    }
}

QImage CharListModel::icon_image(int index) const {
    if (index < 0 || index >= entries_.size())
        return {};
    return entries_[index].icon;
}

// ============================================================================
// QmlUIBridge
// ============================================================================

QmlUIBridge::QmlUIBridge(UIManager& ui_mgr, QObject* parent)
    : QObject(parent), ui_mgr_(ui_mgr) {
}

// --- Sync from engine (called each frame on frameSwapped) ---

void QmlUIBridge::sync_from_engine() {
    // Screen ID
    QString new_id;
    if (auto* screen = ui_mgr_.active_screen())
        new_id = QString::fromStdString(screen->screen_id());

    if (new_id != cached_screen_id_) {
        cached_screen_id_ = new_id;
        emit active_screen_id_changed();
    }

    // Server list screen
    if (auto* sls = dynamic_cast<ServerListScreen*>(ui_mgr_.active_screen())) {
        const auto& servers = sls->get_servers();
        int count = static_cast<int>(servers.size());
        if (count != cached_server_count_) {
            cached_server_count_ = count;
            server_model_.update_from(servers);
        }

        int sel = sls->get_selected();
        if (sel != cached_selected_server_) {
            cached_selected_server_ = sel;
            emit selected_server_changed();
        }
    }

    // Character select screen
    if (auto* css = dynamic_cast<CharSelectScreen*>(ui_mgr_.active_screen())) {
        const auto& chars = css->get_chars();
        int count = static_cast<int>(chars.size());

        if (count != cached_char_count_) {
            cached_char_count_ = count;
            char_model_.reset(count, [&chars](int i) {
                return QString::fromStdString(chars[i].folder);
            });
            emit char_count_changed();
        }
        else if (count > 0) {
            char_model_.refresh_status(count, [&chars](int i) -> CharListModel::CharSnapshot {
                const auto& entry = chars[i];
                CharListModel::CharSnapshot snap;
                snap.taken = entry.taken;
                snap.has_icon = entry.icon.has_value();
                if (snap.has_icon && entry.icon_asset && entry.icon_asset->frame_count() > 0) {
                    const auto& frame = entry.icon_asset->frame(0);
                    const uint8_t* px = entry.icon_asset->frame_pixels(0);
                    QImage img(px, frame.width, frame.height, frame.width * 4, QImage::Format_RGBA8888);
                    snap.image = img.mirrored(false, true);
                }
                return snap;
            });
        }

        int sel = css->get_selected();
        if (sel != cached_selected_char_) {
            cached_selected_char_ = sel;
            emit selected_char_changed();
        }
    }

    // Courtroom screen
    if (auto* cr = dynamic_cast<CourtroomScreen*>(ui_mgr_.active_screen())) {
        QString name = QString::fromStdString(cr->get_character_name());
        if (name != cached_character_name_) {
            cached_character_name_ = name;
            emit character_name_changed();
        }

        bool loading = cr->is_loading();
        if (loading != cached_courtroom_loading_) {
            cached_courtroom_loading_ = loading;
            emit courtroom_loading_changed();
        }

        sync_courtroom_character_data();
        refresh_emote_icons();
    }

    // Poll courtroom events regardless (to drain queued events properly)
    poll_courtroom_events();
}

void QmlUIBridge::sync_courtroom_character_data() {
    auto* cr = dynamic_cast<CourtroomScreen*>(ui_mgr_.active_screen());
    if (!cr)
        return;

    int gen = cr->load_generation();
    if (gen == cached_load_generation_)
        return;

    cached_load_generation_ = gen;
    cached_char_sheet_ = cr->get_character_sheet();

    if (!cached_char_sheet_)
        return;

    // Initialize showname from character sheet
    QString default_showname = QString::fromStdString(cached_char_sheet_->showname());
    if (default_showname != ic_showname_) {
        ic_showname_ = default_showname;
        emit showname_changed();
    }

    // Initialize side from character sheet
    QString side = QString::fromStdString(cached_char_sheet_->side());
    int new_side = 0;
    for (int i = 0; i < 7; i++) {
        if (side == SIDES[i]) {
            new_side = i;
            break;
        }
    }
    if (new_side != ic_side_index_) {
        ic_side_index_ = new_side;
        emit side_index_changed();
    }

    // Rebuild emotes list
    cached_emotes_.clear();
    int count = cached_char_sheet_->emote_count();
    for (int i = 0; i < count; i++) {
        auto emo = cached_char_sheet_->emote(i);
        QVariantMap entry;
        entry[QStringLiteral("comment")] = QString::fromStdString(emo.comment);
        cached_emotes_.append(entry);
    }

    // Cache emote icons
    const auto& icons = cr->get_emote_icons();
    emote_icon_images_.clear();
    emote_icon_images_.resize(static_cast<int>(icons.size()));
    for (int i = 0; i < static_cast<int>(icons.size()); i++) {
        if (icons[i] && icons[i]->frame_count() > 0) {
            const auto& frame = icons[i]->frame(0);
            const uint8_t* px = icons[i]->frame_pixels(0);
            if (px) {
                QImage img(px, frame.width, frame.height, frame.width * 4, QImage::Format_RGBA8888);
                emote_icon_images_[i] = img.mirrored(false, true);
            }
        }
    }
    emote_icon_generation_++;
    emit emotes_changed();
    emit emote_icons_refreshed();

    // Reset selected emote
    ic_selected_emote_ = 0;
    emit selected_emote_changed();
}

void QmlUIBridge::refresh_emote_icons() {
    auto* cr = dynamic_cast<CourtroomScreen*>(ui_mgr_.active_screen());
    if (!cr)
        return;

    const auto& icons = cr->get_emote_icons();
    bool changed = false;
    for (int i = 0; i < static_cast<int>(icons.size()) && i < emote_icon_images_.size(); i++) {
        if (emote_icon_images_[i].isNull() && icons[i] && icons[i]->frame_count() > 0) {
            const auto& frame = icons[i]->frame(0);
            const uint8_t* px = icons[i]->frame_pixels(0);
            if (px) {
                QImage img(px, frame.width, frame.height, frame.width * 4, QImage::Format_RGBA8888);
                emote_icon_images_[i] = img.mirrored(false, true);
                changed = true;
            }
        }
    }
    if (changed) {
        emote_icon_generation_++;
        emit emote_icons_refreshed();
    }
}

void QmlUIBridge::poll_courtroom_events() {
    // IC log
    {
        auto& ch = EventManager::instance().get_channel<ICLogEvent>();
        bool changed = false;
        while (auto ev = ch.get_event()) {
            if (!ic_log_.isEmpty())
                ic_log_ += '\n';
            ic_log_ += QStringLiteral("[%1] %2").arg(QString::fromStdString(ev->get_showname()),
                                                      QString::fromStdString(ev->get_message()));
            changed = true;
        }
        if (changed)
            emit ic_log_changed();
    }

    // OOC chat
    {
        auto& ch = EventManager::instance().get_channel<ChatEvent>();
        bool changed = false;
        while (auto ev = ch.get_event()) {
            if (!ooc_log_.isEmpty())
                ooc_log_ += '\n';
            ooc_log_ += QString::fromStdString(ev->to_string());
            changed = true;
        }
        if (changed)
            emit ooc_log_changed();
    }

    // Music list
    {
        auto& ch = EventManager::instance().get_channel<MusicListEvent>();
        bool tracks_changed = false;
        bool areas_changed_flag = false;
        while (auto ev = ch.get_event()) {
            if (ev->partial()) {
                if (!ev->areas().empty()) {
                    area_entries_.clear();
                    for (const auto& a : ev->areas()) {
                        AreaEntry ae;
                        ae.name = QString::fromStdString(a);
                        area_entries_.push_back(ae);
                    }
                    areas_changed_flag = true;
                }
                if (!ev->tracks().empty()) {
                    cached_music_tracks_.clear();
                    for (const auto& t : ev->tracks()) {
                        QVariantMap entry;
                        entry[QStringLiteral("name")] = QString::fromStdString(t);
                        entry[QStringLiteral("isCategory")] = !t.empty() && t.find('.') == std::string::npos;
                        cached_music_tracks_.append(entry);
                    }
                    tracks_changed = true;
                }
            }
            else {
                area_entries_.clear();
                for (const auto& a : ev->areas()) {
                    AreaEntry ae;
                    ae.name = QString::fromStdString(a);
                    area_entries_.push_back(ae);
                }
                areas_changed_flag = true;

                cached_music_tracks_.clear();
                for (const auto& t : ev->tracks()) {
                    QVariantMap entry;
                    entry[QStringLiteral("name")] = QString::fromStdString(t);
                    entry[QStringLiteral("isCategory")] = !t.empty() && t.find('.') == std::string::npos;
                    cached_music_tracks_.append(entry);
                }
                tracks_changed = true;
            }
        }
        if (tracks_changed)
            emit music_tracks_changed();
        if (areas_changed_flag) {
            // Rebuild areas QVariantList
            cached_areas_.clear();
            for (const auto& ae : area_entries_) {
                QVariantMap m;
                m[QStringLiteral("name")] = ae.name;
                m[QStringLiteral("players")] = ae.players;
                m[QStringLiteral("status")] = ae.status;
                m[QStringLiteral("cm")] = ae.cm;
                m[QStringLiteral("locked")] = ae.locked;
                cached_areas_.append(m);
            }
            emit areas_changed();
        }
    }

    // Area updates (ARUP)
    {
        auto& ch = EventManager::instance().get_channel<AreaUpdateEvent>();
        bool changed = false;
        while (auto ev = ch.get_event()) {
            const auto& vals = ev->values();
            size_t count = std::min(vals.size(), area_entries_.size());
            switch (ev->type()) {
            case AreaUpdateEvent::PLAYERS:
                for (size_t i = 0; i < count; i++)
                    area_entries_[i].players = std::atoi(vals[i].c_str());
                break;
            case AreaUpdateEvent::STATUS:
                for (size_t i = 0; i < count; i++)
                    area_entries_[i].status = QString::fromStdString(vals[i]);
                break;
            case AreaUpdateEvent::CM:
                for (size_t i = 0; i < count; i++)
                    area_entries_[i].cm = QString::fromStdString(vals[i]);
                break;
            case AreaUpdateEvent::LOCK:
                for (size_t i = 0; i < count; i++)
                    area_entries_[i].locked = (vals[i] == "LOCKED");
                break;
            }
            changed = true;
        }
        if (changed) {
            cached_areas_.clear();
            for (const auto& ae : area_entries_) {
                QVariantMap m;
                m[QStringLiteral("name")] = ae.name;
                m[QStringLiteral("players")] = ae.players;
                m[QStringLiteral("status")] = ae.status;
                m[QStringLiteral("cm")] = ae.cm;
                m[QStringLiteral("locked")] = ae.locked;
                cached_areas_.append(m);
            }
            emit areas_changed();
        }
    }

    // Now playing
    {
        auto& ch = EventManager::instance().get_channel<NowPlayingEvent>();
        while (auto ev = ch.get_event()) {
            QString track = QString::fromStdString(ev->track());
            if (track != now_playing_) {
                now_playing_ = track;
                emit now_playing_changed();
            }
        }
    }

    // Evidence
    {
        auto& ch = EventManager::instance().get_channel<EvidenceListEvent>();
        while (auto ev = ch.get_event()) {
            cached_evidence_.clear();
            for (const auto& item : ev->items()) {
                QVariantMap m;
                m[QStringLiteral("name")] = QString::fromStdString(item.name);
                m[QStringLiteral("description")] = QString::fromStdString(item.description);
                m[QStringLiteral("image")] = QString::fromStdString(item.image);
                cached_evidence_.append(m);
            }
            emit evidence_changed();
        }
    }

    // Health bars
    {
        auto& ch = EventManager::instance().get_channel<HealthBarEvent>();
        bool changed = false;
        while (auto ev = ch.get_event()) {
            int val = std::clamp(ev->value(), 0, 10);
            if (ev->side() == 1 && val != def_hp_) {
                def_hp_ = val;
                changed = true;
            }
            if (ev->side() == 2 && val != pro_hp_) {
                pro_hp_ = val;
                changed = true;
            }
        }
        if (changed)
            emit hp_changed();
    }

    // Player list
    {
        auto& ch = EventManager::instance().get_channel<PlayerListEvent>();
        bool changed = false;
        while (auto ev = ch.get_event()) {
            int id = ev->player_id();
            switch (ev->action()) {
            case PlayerListEvent::Action::ADD:
                players_map_[id] = {};
                break;
            case PlayerListEvent::Action::REMOVE:
                players_map_.erase(id);
                break;
            case PlayerListEvent::Action::UPDATE_NAME:
                players_map_[id].name = QString::fromStdString(ev->data());
                break;
            case PlayerListEvent::Action::UPDATE_CHARACTER:
                players_map_[id].character = QString::fromStdString(ev->data());
                break;
            case PlayerListEvent::Action::UPDATE_CHARNAME:
                players_map_[id].charname = QString::fromStdString(ev->data());
                break;
            case PlayerListEvent::Action::UPDATE_AREA:
                players_map_[id].area_id = std::atoi(ev->data().c_str());
                break;
            }
            changed = true;
        }
        if (changed)
            rebuild_players_list();
    }

    // Timers
    {
        auto& ch = EventManager::instance().get_channel<TimerEvent>();
        while (auto ev = ch.get_event()) {
            int id = ev->timer_id();
            if (id < 0 || id >= MAX_TIMERS)
                continue;

            auto& t = timers_[id];
            switch (ev->action()) {
            case 0: // start/sync
                if (ev->time_ms() < 0) {
                    t.running = false;
                    t.remaining_ms = 0;
                }
                else {
                    t.remaining_ms = ev->time_ms();
                    t.running = true;
                    t.last_tick = std::chrono::steady_clock::now();
                }
                break;
            case 1: // pause
                t.running = false;
                t.remaining_ms = ev->time_ms();
                break;
            case 2: // show
                t.visible = true;
                break;
            case 3: // hide
                t.visible = false;
                break;
            }
        }
        update_timer_display();
    }
}

void QmlUIBridge::rebuild_players_list() {
    cached_players_.clear();
    for (const auto& [id, info] : players_map_) {
        QString display;
        if (!info.charname.isEmpty())
            display = info.charname;
        else if (!info.character.isEmpty())
            display = info.character;

        if (!info.name.isEmpty()) {
            if (display.isEmpty())
                display = info.name;
            else
                display += QStringLiteral(" (%1)").arg(info.name);
        }

        if (display.isEmpty())
            display = QStringLiteral("Player %1").arg(id);

        cached_players_.append(display);
    }
    emit players_changed();
}

void QmlUIBridge::update_timer_display() {
    auto now = std::chrono::steady_clock::now();
    QString display;

    for (int i = 0; i < MAX_TIMERS; i++) {
        auto& t = timers_[i];
        if (!t.visible)
            continue;

        if (t.running) {
            auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - t.last_tick).count();
            t.last_tick = now;
            t.remaining_ms -= elapsed;
            if (t.remaining_ms < 0)
                t.remaining_ms = 0;
        }

        int64_t ms = t.remaining_ms;
        int hours = static_cast<int>(ms / 3600000);
        int minutes = static_cast<int>((ms % 3600000) / 60000);
        int seconds = static_cast<int>((ms % 60000) / 1000);
        int centis = static_cast<int>((ms % 1000) / 10);

        if (!display.isEmpty())
            display += '\n';

        if (hours > 0)
            display += QStringLiteral("Timer %1: %2:%3:%4.%5")
                           .arg(i)
                           .arg(hours)
                           .arg(minutes, 2, 10, QChar('0'))
                           .arg(seconds, 2, 10, QChar('0'))
                           .arg(centis, 2, 10, QChar('0'));
        else
            display += QStringLiteral("Timer %1: %2:%3.%4")
                           .arg(i)
                           .arg(minutes, 2, 10, QChar('0'))
                           .arg(seconds, 2, 10, QChar('0'))
                           .arg(centis, 2, 10, QChar('0'));
    }

    if (display != cached_timer_display_) {
        cached_timer_display_ = display;
        emit timer_display_changed();
    }
}

// ============================================================================
// Property accessors — screen navigation
// ============================================================================

QString QmlUIBridge::active_screen_id() const {
    return cached_screen_id_;
}

ServerListModel* QmlUIBridge::server_list_model() {
    return &server_model_;
}

int QmlUIBridge::selected_server() const {
    return cached_selected_server_;
}

CharListModel* QmlUIBridge::char_list_model() {
    return &char_model_;
}

int QmlUIBridge::selected_char() const {
    return cached_selected_char_;
}

int QmlUIBridge::char_count() const {
    return cached_char_count_;
}

QString QmlUIBridge::character_name() const {
    return cached_character_name_;
}

bool QmlUIBridge::courtroom_loading() const {
    return cached_courtroom_loading_;
}

// ============================================================================
// Property accessors — IC message state
// ============================================================================

QString QmlUIBridge::showname() const { return ic_showname_; }
void QmlUIBridge::set_showname(const QString& v) {
    if (ic_showname_ != v) { ic_showname_ = v; emit showname_changed(); }
}

int QmlUIBridge::selected_emote_idx() const { return ic_selected_emote_; }
void QmlUIBridge::set_selected_emote(int v) {
    if (ic_selected_emote_ != v) { ic_selected_emote_ = v; emit selected_emote_changed(); }
}

int QmlUIBridge::objection_mod() const { return ic_objection_mod_; }
void QmlUIBridge::set_objection_mod(int v) {
    if (ic_objection_mod_ != v) { ic_objection_mod_ = v; emit objection_mod_changed(); }
}

int QmlUIBridge::side_index() const { return ic_side_index_; }
void QmlUIBridge::set_side_index(int v) {
    if (ic_side_index_ != v) { ic_side_index_ = v; emit side_index_changed(); }
}

int QmlUIBridge::text_color() const { return ic_text_color_; }
void QmlUIBridge::set_text_color(int v) {
    if (ic_text_color_ != v) { ic_text_color_ = v; emit text_color_changed(); }
}

bool QmlUIBridge::flip() const { return ic_flip_; }
void QmlUIBridge::set_flip(bool v) {
    if (ic_flip_ != v) { ic_flip_ = v; emit flip_changed(); }
}

bool QmlUIBridge::pre_anim() const { return ic_pre_anim_; }
void QmlUIBridge::set_pre_anim(bool v) {
    if (ic_pre_anim_ != v) { ic_pre_anim_ = v; emit pre_anim_changed(); }
}

bool QmlUIBridge::realization() const { return ic_realization_; }
void QmlUIBridge::set_realization(bool v) {
    if (ic_realization_ != v) { ic_realization_ = v; emit realization_changed(); }
}

bool QmlUIBridge::screenshake() const { return ic_screenshake_; }
void QmlUIBridge::set_screenshake(bool v) {
    if (ic_screenshake_ != v) { ic_screenshake_ = v; emit screenshake_changed(); }
}

bool QmlUIBridge::additive() const { return ic_additive_; }
void QmlUIBridge::set_additive(bool v) {
    if (ic_additive_ != v) { ic_additive_ = v; emit additive_changed(); }
}

// ============================================================================
// Property accessors — courtroom data
// ============================================================================

QString QmlUIBridge::ic_log() const { return ic_log_; }
QString QmlUIBridge::ooc_log() const { return ooc_log_; }
QString QmlUIBridge::now_playing() const { return now_playing_; }
int QmlUIBridge::def_hp() const { return def_hp_; }
int QmlUIBridge::pro_hp() const { return pro_hp_; }
QString QmlUIBridge::timer_display() const { return cached_timer_display_; }
QVariantList QmlUIBridge::music_tracks() const { return cached_music_tracks_; }
QVariantList QmlUIBridge::areas_data() const { return cached_areas_; }
QVariantList QmlUIBridge::evidence_data() const { return cached_evidence_; }
QVariantList QmlUIBridge::players_data() const { return cached_players_; }
QVariantList QmlUIBridge::emotes_data() const { return cached_emotes_; }
int QmlUIBridge::emote_icon_generation() const { return emote_icon_generation_; }

QImage QmlUIBridge::emote_icon_image(int index) const {
    if (index < 0 || index >= emote_icon_images_.size())
        return {};
    return emote_icon_images_[index];
}

// ============================================================================
// Actions
// ============================================================================

void QmlUIBridge::select_server(int index) {
    if (auto* sls = dynamic_cast<ServerListScreen*>(ui_mgr_.active_screen()))
        sls->select_server(index);
}

void QmlUIBridge::direct_connect(const QString& host, int port) {
    if (auto* sls = dynamic_cast<ServerListScreen*>(ui_mgr_.active_screen()))
        sls->direct_connect(host.toStdString(), static_cast<uint16_t>(port));
}

void QmlUIBridge::select_character(int index) {
    if (auto* css = dynamic_cast<CharSelectScreen*>(ui_mgr_.active_screen()))
        css->select_character(index);
}

void QmlUIBridge::request_disconnect() {
    // Clear courtroom state on disconnect
    ic_log_.clear();
    ooc_log_.clear();
    now_playing_.clear();
    def_hp_ = 0;
    pro_hp_ = 0;
    cached_music_tracks_.clear();
    area_entries_.clear();
    cached_areas_.clear();
    cached_evidence_.clear();
    players_map_.clear();
    cached_players_.clear();
    cached_emotes_.clear();
    emote_icon_images_.clear();
    cached_load_generation_ = -1;
    cached_char_sheet_.reset();
    for (int i = 0; i < MAX_TIMERS; i++)
        timers_[i] = {};
    cached_timer_display_.clear();

    ui_mgr_.pop_to_root();
}

void QmlUIBridge::pop_screen() {
    ui_mgr_.pop_screen();
}

void QmlUIBridge::send_ic_message(const QString& message) {
    if (message.isEmpty())
        return;

    auto* cr = dynamic_cast<CourtroomScreen*>(ui_mgr_.active_screen());
    if (!cr)
        return;

    ICMessageData data;
    data.character = cr->get_character_name();
    data.char_id = cr->get_char_id();
    data.message = message.toStdString();
    data.showname = ic_showname_.toStdString();

    auto sheet = cr->get_character_sheet();
    if (sheet && ic_selected_emote_ >= 0 && ic_selected_emote_ < sheet->emote_count()) {
        const auto& emo = sheet->emote(ic_selected_emote_);
        data.emote = emo.anim_name;
        data.pre_emote = emo.pre_anim;
        data.desk_mod = emo.desk_mod;
        if (!emo.sfx_name.empty() && emo.sfx_name != "0") {
            data.sfx_name = emo.sfx_name;
            data.sfx_delay = emo.sfx_delay;
        }
    }

    data.emote_mod = ic_pre_anim_ ? 1 : 0;
    data.side = SIDES[std::clamp(ic_side_index_, 0, 6)];
    data.objection_mod = ic_objection_mod_;
    data.flip = ic_flip_ ? 1 : 0;
    data.text_color = ic_text_color_;
    data.realization = ic_realization_ ? 1 : 0;
    data.screenshake = ic_screenshake_ ? 1 : 0;
    data.additive = ic_additive_ ? 1 : 0;

    EventManager::instance().get_channel<OutgoingICMessageEvent>().publish(
        OutgoingICMessageEvent(std::move(data)));

    // Reset one-shot state after send
    ic_objection_mod_ = 0;
    emit objection_mod_changed();
    ic_realization_ = false;
    emit realization_changed();
    ic_screenshake_ = false;
    emit screenshake_changed();
}

void QmlUIBridge::send_ooc_message(const QString& name, const QString& message) {
    if (message.isEmpty())
        return;

    EventManager::instance().get_channel<OutgoingChatEvent>().publish(
        OutgoingChatEvent(name.toStdString(), message.toStdString()));
}

void QmlUIBridge::play_music(const QString& track) {
    EventManager::instance().get_channel<OutgoingMusicEvent>().publish(
        OutgoingMusicEvent(track.toStdString(), ic_showname_.toStdString()));
}

void QmlUIBridge::set_health(int side, int value) {
    value = std::clamp(value, 0, 10);
    EventManager::instance().get_channel<OutgoingHealthBarEvent>().publish(
        OutgoingHealthBarEvent(side, value));
}

void QmlUIBridge::set_volume(int category, float value) {
    // Cubic curve: slider 0-100 → amplitude
    float t = std::clamp(value / 100.0f, 0.0f, 1.0f);
    float amplitude = t * t * t;

    VolumeChangeEvent::Category cat;
    switch (category) {
    case 0:
        cat = VolumeChangeEvent::Category::MUSIC;
        break;
    case 1:
        cat = VolumeChangeEvent::Category::SFX;
        break;
    case 2:
        cat = VolumeChangeEvent::Category::BLIP;
        break;
    default:
        return;
    }

    EventManager::instance().get_channel<VolumeChangeEvent>().publish(
        VolumeChangeEvent(cat, amplitude));
}

void QmlUIBridge::select_emote(int index) {
    if (index == ic_selected_emote_)
        return;

    ic_selected_emote_ = index;
    emit selected_emote_changed();

    // Auto-set pre-animation flag based on emote data
    if (cached_char_sheet_ && index >= 0 && index < cached_char_sheet_->emote_count()) {
        const auto& emo = cached_char_sheet_->emote(index);
        bool has_pre = !emo.pre_anim.empty() && emo.pre_anim != "-";
        if (has_pre != ic_pre_anim_) {
            ic_pre_anim_ = has_pre;
            emit pre_anim_changed();
        }
    }
}

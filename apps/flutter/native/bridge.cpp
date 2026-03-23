/**
 * @file bridge.cpp
 * @brief C API bridge implementation — wires Flutter into the AO-SDL engine.
 *
 * Mirrors the structure of apps/sdl/main.cpp but exposes a polling API
 * instead of running its own event loop. Flutter drives the frame cadence;
 * this bridge just pumps the engine subsystems on each ao_tick().
 */
#include "bridge.h"

// Engine
#include "ao/asset/AOAssetLibrary.h"
#include "asset/AssetLibrary.h"
#include "asset/MediaManager.h"
#include "asset/MountHttp.h"
#include "asset/MountManager.h"
#include "audio/AudioThread.h"
#include "event/AreaUpdateEvent.h"
#include "event/AssetUrlEvent.h"
#include "event/ChatEvent.h"
#include "event/DisconnectEvent.h"
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
#include "event/PlayerCountEvent.h"
#include "event/PlayerListEvent.h"
#include "event/ServerInfoEvent.h"
#include "event/ServerListEvent.h"
#include "event/TimerEvent.h"
#include "event/VolumeChangeEvent.h"
#include "game/GameThread.h"
#include "game/ServerList.h"
#include "net/HttpPool.h"
#include "net/NetworkThread.h"
#include "render/IRenderer.h"
#include "render/RenderManager.h"
#include "render/StateBuffer.h"
#include "ui/UIManager.h"
#include "utils/Log.h"

// Plugins
#include "ao/ao_plugin.h"
#include "ao/ui/screens/CharSelectScreen.h"
#include "ao/ui/screens/CourtroomScreen.h"
#include "ao/ui/screens/ServerListScreen.h"

#include <algorithm>
#include <chrono>
#include <csignal>
#include <filesystem>
#include <map>
#include <memory>
#include <string>
#include <vector>

// Provided by the linked render plugin (aorender_gl or aorender_metal).
std::unique_ptr<IRenderer> create_renderer(int width, int height);

// ---------------------------------------------------------------------------
// Globals — same lifetime objects as apps/sdl/main.cpp, but heap-allocated
// so we can init/shutdown from FFI calls.
// ---------------------------------------------------------------------------
namespace {

struct Engine {
    std::unique_ptr<HttpPool> http_pool;
    UIManager ui_mgr;
    StateBuffer buffer;
    std::unique_ptr<ProtocolHandler> protocol;
    std::unique_ptr<NetworkThread> net_thread;
    std::unique_ptr<RenderManager> render_mgr;
    std::unique_ptr<IScenePresenter> presenter;
    std::unique_ptr<GameThread> game_thread;
    // TODO: AudioThread + mobile audio device.  The SDL app creates
    // SDLAudioDevice + AudioThread for blips/sfx/music.  For Flutter/mobile we
    // need a platform audio device (e.g. AAudio on Android, AVAudioEngine on
    // iOS) before the AudioThread can be started.  Until then no audio plays.
    bool default_mount_added = false;
};

std::unique_ptr<Engine> g_engine;

// Cached OOC/IC log entries for Flutter to read
struct ChatMsg {
    std::string name;
    std::string text;
};
std::vector<ChatMsg> g_ooc_msgs;
std::vector<ChatMsg> g_ic_log;

// Music & Area lists (from MusicListEvent)
std::vector<std::string> g_areas;
std::vector<std::string> g_tracks;
std::vector<int> g_area_players;
std::vector<std::string> g_area_status;
std::vector<std::string> g_area_cm;
std::vector<std::string> g_area_lock;
std::string g_now_playing;

// Disconnect state
bool g_disconnect_pending = false;
std::string g_disconnect_reason;

// Player list (from PlayerListEvent)
struct PlayerInfo {
    int id;
    std::string name;
    std::string character;
    std::string charname;
    int area_id = -1;
};
std::map<int, PlayerInfo> g_players;
std::vector<int> g_player_ids_cache; // sorted snapshot for indexed access

// Evidence (from EvidenceListEvent)
std::vector<EvidenceItem> g_evidence;

// Health bars (from HealthBarEvent)
int g_def_hp = 0;
int g_pro_hp = 0;

// Timers (from TimerEvent)
static constexpr int MAX_TIMERS = 4;
struct TimerState {
    bool visible = false;
    bool running = false;
    int64_t remaining_ms = 0;
    std::chrono::steady_clock::time_point last_tick;
};
TimerState g_timers[MAX_TIMERS];

// Server info (from ServerInfoEvent)
std::string g_server_software;
std::string g_server_version;
int g_server_player_num = -1;

// Player count (from PlayerCountEvent)
int g_pcount_current = 0;
int g_pcount_max = 0;
std::string g_pcount_description;

// Scratch buffer for returning strings from functions that return by value.
// Keeps the string alive until the next call to the same function.
std::string g_scratch;

// Emote icon cache — retries loading icons that were null during initial load.
// Indexed by emote index; populated lazily.
std::vector<std::shared_ptr<ImageAsset>> g_emote_icon_cache;
std::string g_emote_icon_character; // character these icons belong to

// IC chat state — mirrors ICMessageState from the SDL app
struct ICState {
    int selected_emote = 0;
    int side_index = 2; // default: wit
    std::string showname;
    bool pre_anim = false;
    bool flip = false;
    int objection_mod = 0;
    int text_color = 0;
};
ICState g_ic_state;

constexpr const char* SIDES[] = {"def", "pro", "wit", "jud", "jur", "sea", "hlp"};
constexpr int SIDES_COUNT = 7;

} // namespace

// ---------------------------------------------------------------------------
// Helpers — get typed screen pointers from the active screen
// ---------------------------------------------------------------------------
static ServerListScreen* as_server_list() {
    auto* s = g_engine ? g_engine->ui_mgr.active_screen() : nullptr;
    return (s && s->screen_id() == ServerListScreen::ID) ? static_cast<ServerListScreen*>(s) : nullptr;
}

static CharSelectScreen* as_char_select() {
    auto* s = g_engine ? g_engine->ui_mgr.active_screen() : nullptr;
    return (s && s->screen_id() == CharSelectScreen::ID) ? static_cast<CharSelectScreen*>(s) : nullptr;
}

static CourtroomScreen* as_courtroom() {
    auto* s = g_engine ? g_engine->ui_mgr.active_screen() : nullptr;
    return (s && s->screen_id() == CourtroomScreen::ID) ? static_cast<CourtroomScreen*>(s) : nullptr;
}

// ---------------------------------------------------------------------------
// Lifecycle
// ---------------------------------------------------------------------------

void ao_init(const char* base_path) {
    if (g_engine)
        return;

#ifndef _WIN32
    std::signal(SIGPIPE, SIG_IGN);
#endif

    g_engine = std::make_unique<Engine>();

    // HTTP pool
    g_engine->http_pool = std::make_unique<HttpPool>(8);
    g_engine->http_pool->get("http://servers.aceattorneyonline.com", "/servers", [](HttpResponse resp) {
        if (resp.status == 200) {
            ServerList svlist(resp.body);
            EventManager::instance().get_channel<ServerListEvent>().publish(ServerListEvent(svlist));
        }
        else {
            Log::log_print(ERR, "Failed to fetch server list: %s", resp.error.c_str());
        }
    });

    // Mount local assets
    if (base_path) {
        std::filesystem::path base_dir(base_path);
        if (std::filesystem::is_directory(base_dir)) {
            MediaManager::instance().init(base_dir);
            Log::log_print(INFO, "Mounted local content: %s", base_dir.c_str());
        }
    }

    // Initial screen
    g_engine->ui_mgr.push_screen(std::make_unique<ServerListScreen>());

    // Protocol + network
    g_engine->protocol = ao::create_protocol();
    g_engine->net_thread = std::make_unique<NetworkThread>(*g_engine->protocol);

    // Scene presenter + game thread (created but renderer may not exist yet)
    g_engine->presenter = ao::create_presenter();

    Log::log_print(INFO, "ao_init: engine initialized");
}

void ao_shutdown() {
    if (!g_engine)
        return;

    if (g_engine->net_thread)
        g_engine->net_thread->stop();
    if (g_engine->game_thread)
        g_engine->game_thread->stop();
    if (g_engine->http_pool)
        g_engine->http_pool->stop();
    MediaManager::instance().shutdown();

    g_engine.reset();
    g_ooc_msgs.clear();
    g_ic_log.clear();
    g_areas.clear();
    g_tracks.clear();
    g_area_players.clear();
    g_area_status.clear();
    g_area_cm.clear();
    g_area_lock.clear();
    g_now_playing.clear();
    g_disconnect_pending = false;
    g_disconnect_reason.clear();
    g_players.clear();
    g_player_ids_cache.clear();
    g_evidence.clear();
    g_def_hp = 0;
    g_pro_hp = 0;
    for (int i = 0; i < MAX_TIMERS; i++)
        g_timers[i] = {};
    g_server_software.clear();
    g_server_version.clear();
    g_server_player_num = -1;
    g_pcount_current = 0;
    g_pcount_max = 0;
    g_pcount_description.clear();

    Log::log_print(INFO, "ao_shutdown: complete");
}

// ---------------------------------------------------------------------------
// Frame pump
// ---------------------------------------------------------------------------

void ao_tick() {
    if (!g_engine)
        return;

    g_engine->http_pool->poll();

    // Handle asset URL events (ASS packets) — same as SDL main.cpp
    auto& asset_ch = EventManager::instance().get_channel<AssetUrlEvent>();
    while (auto ev = asset_ch.get_event()) {
        auto mount = std::make_unique<MountHttp>(ev->url(), *g_engine->http_pool);
        MediaManager::instance().mounts_ref().add_mount(std::move(mount));
        Log::log_print(INFO, "Added HTTP asset mount: %s", ev->url().c_str());

        if (!g_engine->default_mount_added) {
            g_engine->default_mount_added = true;
            auto fallback = std::make_unique<MountHttp>("https://attorneyoffline.de/base/", *g_engine->http_pool);
            MediaManager::instance().mounts_ref().add_mount(std::move(fallback));
        }
    }

    g_engine->ui_mgr.handle_events();

    // --- Consume ChatEvent (OOC messages) ---
    {
        auto& ch = EventManager::instance().get_channel<ChatEvent>();
        while (auto ev = ch.get_event()) {
            g_ooc_msgs.push_back({ev->get_sender_name(), ev->get_message()});
        }
    }

    // --- Consume ICLogEvent ---
    {
        auto& ch = EventManager::instance().get_channel<ICLogEvent>();
        while (auto ev = ch.get_event()) {
            g_ic_log.push_back({ev->get_showname(), ev->get_message()});
        }
    }

    // --- Consume DisconnectEvent ---
    {
        auto& ch = EventManager::instance().get_channel<DisconnectEvent>();
        while (auto ev = ch.get_event()) {
            g_disconnect_pending = true;
            g_disconnect_reason = ev->get_reason();
        }
    }

    // --- Consume MusicListEvent ---
    {
        auto& ch = EventManager::instance().get_channel<MusicListEvent>();
        while (auto ev = ch.get_event()) {
            if (ev->partial()) {
                if (!ev->areas().empty()) {
                    g_areas = ev->areas();
                    size_t n = g_areas.size();
                    g_area_players.assign(n, -1);
                    g_area_status.assign(n, "Unknown");
                    g_area_cm.assign(n, "Unknown");
                    g_area_lock.assign(n, "Unknown");
                }
                if (!ev->tracks().empty()) {
                    g_tracks = ev->tracks();
                }
            }
            else {
                g_areas = ev->areas();
                g_tracks = ev->tracks();
                size_t n = g_areas.size();
                g_area_players.assign(n, -1);
                g_area_status.assign(n, "Unknown");
                g_area_cm.assign(n, "Unknown");
                g_area_lock.assign(n, "Unknown");
            }
        }
    }

    // --- Consume AreaUpdateEvent ---
    {
        auto& ch = EventManager::instance().get_channel<AreaUpdateEvent>();
        while (auto ev = ch.get_event()) {
            const auto& vals = ev->values();
            size_t count = std::min(vals.size(), g_areas.size());
            switch (ev->type()) {
            case AreaUpdateEvent::PLAYERS:
                for (size_t i = 0; i < count; i++)
                    g_area_players[i] = std::atoi(vals[i].c_str());
                break;
            case AreaUpdateEvent::STATUS:
                for (size_t i = 0; i < count; i++)
                    g_area_status[i] = vals[i];
                break;
            case AreaUpdateEvent::CM:
                for (size_t i = 0; i < count; i++)
                    g_area_cm[i] = vals[i];
                break;
            case AreaUpdateEvent::LOCK:
                for (size_t i = 0; i < count; i++)
                    g_area_lock[i] = vals[i];
                break;
            }
        }
    }

    // --- Consume NowPlayingEvent ---
    {
        auto& ch = EventManager::instance().get_channel<NowPlayingEvent>();
        while (auto ev = ch.get_event()) {
            g_now_playing = ev->track();
        }
    }

    // --- Consume PlayerListEvent ---
    {
        auto& ch = EventManager::instance().get_channel<PlayerListEvent>();
        bool changed = false;
        while (auto ev = ch.get_event()) {
            changed = true;
            int id = ev->player_id();
            switch (ev->action()) {
            case PlayerListEvent::Action::ADD:
                g_players[id] = {id, {}, {}, {}, -1};
                break;
            case PlayerListEvent::Action::REMOVE:
                g_players.erase(id);
                break;
            case PlayerListEvent::Action::UPDATE_NAME:
                g_players[id].name = ev->data();
                break;
            case PlayerListEvent::Action::UPDATE_CHARACTER:
                g_players[id].character = ev->data();
                break;
            case PlayerListEvent::Action::UPDATE_CHARNAME:
                g_players[id].charname = ev->data();
                break;
            case PlayerListEvent::Action::UPDATE_AREA:
                g_players[id].area_id = std::atoi(ev->data().c_str());
                break;
            }
        }
        if (changed) {
            g_player_ids_cache.clear();
            g_player_ids_cache.reserve(g_players.size());
            for (const auto& [id, _] : g_players)
                g_player_ids_cache.push_back(id);
        }
    }

    // --- Consume EvidenceListEvent ---
    {
        auto& ch = EventManager::instance().get_channel<EvidenceListEvent>();
        while (auto ev = ch.get_event()) {
            g_evidence = ev->items();
        }
    }

    // --- Consume HealthBarEvent ---
    {
        auto& ch = EventManager::instance().get_channel<HealthBarEvent>();
        while (auto ev = ch.get_event()) {
            int val = std::clamp(ev->value(), 0, 10);
            if (ev->side() == 1)
                g_def_hp = val;
            else if (ev->side() == 2)
                g_pro_hp = val;
        }
    }

    // --- Consume TimerEvent ---
    {
        auto& ch = EventManager::instance().get_channel<TimerEvent>();
        while (auto ev = ch.get_event()) {
            int id = ev->timer_id();
            if (id < 0 || id >= MAX_TIMERS)
                continue;
            auto& t = g_timers[id];
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
    }

    // --- Tick down running timers ---
    {
        auto now = std::chrono::steady_clock::now();
        for (int i = 0; i < MAX_TIMERS; i++) {
            auto& t = g_timers[i];
            if (t.running) {
                auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - t.last_tick).count();
                t.last_tick = now;
                t.remaining_ms -= elapsed;
                if (t.remaining_ms < 0)
                    t.remaining_ms = 0;
            }
        }
    }

    // --- Consume ServerInfoEvent ---
    {
        auto& ch = EventManager::instance().get_channel<ServerInfoEvent>();
        while (auto ev = ch.get_event()) {
            g_server_software = ev->get_software();
            g_server_version = ev->get_version();
            g_server_player_num = ev->get_player_num();
        }
    }

    // --- Consume PlayerCountEvent ---
    {
        auto& ch = EventManager::instance().get_channel<PlayerCountEvent>();
        while (auto ev = ch.get_event()) {
            g_pcount_current = ev->get_current();
            g_pcount_max = ev->get_max();
            g_pcount_description = ev->get_description();
        }
    }

    // Retry loading emote icons that were null during initial courtroom load.
    auto* cr = as_courtroom();
    if (cr && !cr->is_loading() && cr->get_character_sheet()) {
        const auto& name = cr->get_character_name();
        int count = cr->get_character_sheet()->emote_count();

        // Reset cache and IC state if character changed
        if (name != g_emote_icon_character) {
            g_emote_icon_cache.clear();
            g_emote_icon_cache.resize(count);
            g_emote_icon_character = name;

            // Auto-populate showname from char.ini (Issue 5)
            g_ic_state.showname = cr->get_character_sheet()->showname();

            // Auto-populate side/position from char.ini (Issue 7)
            const auto& side_str = cr->get_character_sheet()->side();
            g_ic_state.side_index = 2; // default: wit
            for (int i = 0; i < 7; i++) {
                if (side_str == SIDES[i]) {
                    g_ic_state.side_index = i;
                    break;
                }
            }

            // Reset emote selection
            g_ic_state.selected_emote = 0;
        }

        // Try to load a few missing icons per tick
        AOAssetLibrary ao_assets(MediaManager::instance().assets());
        int loaded = 0;
        for (int i = 0; i < count && loaded < 4; i++) {
            if (g_emote_icon_cache[i])
                continue;
            auto asset = ao_assets.emote_icon(name, i);
            if (asset && asset->frame_count() > 0) {
                g_emote_icon_cache[i] = asset;
                loaded++;
            }
        }
    }
}

// ---------------------------------------------------------------------------
// Renderer
// ---------------------------------------------------------------------------

void ao_renderer_create(int width, int height, bool use_metal) {
    if (!g_engine)
        return;
    (void)use_metal; // Renderer selection is at link time

    auto renderer = create_renderer(width, height);
    MediaManager::instance().assets().set_shader_backend(renderer->backend_name());
    g_engine->render_mgr = std::make_unique<RenderManager>(g_engine->buffer, std::move(renderer));

    // Now that the renderer exists, start the game thread
    if (!g_engine->game_thread) {
        g_engine->game_thread = std::make_unique<GameThread>(g_engine->buffer, *g_engine->presenter);
    }
}

void ao_renderer_draw() {
    if (!g_engine || !g_engine->render_mgr)
        return;
    g_engine->render_mgr->render_frame();
}

uintptr_t ao_renderer_get_texture() {
    if (!g_engine || !g_engine->render_mgr)
        return 0;
    return g_engine->render_mgr->get_renderer().get_render_texture_id();
}

void ao_renderer_resize(int width, int height) {
    if (!g_engine || !g_engine->render_mgr)
        return;
    g_engine->render_mgr->get_renderer().resize(width, height);
}

void* ao_renderer_get_metal_device() {
    if (!g_engine || !g_engine->render_mgr)
        return nullptr;
    return g_engine->render_mgr->get_renderer().get_device_ptr();
}

void* ao_renderer_get_metal_command_queue() {
    if (!g_engine || !g_engine->render_mgr)
        return nullptr;
    return g_engine->render_mgr->get_renderer().get_command_queue_ptr();
}

// ---------------------------------------------------------------------------
// Screen state
// ---------------------------------------------------------------------------

const char* ao_active_screen_id() {
    if (!g_engine)
        return "";
    auto* s = g_engine->ui_mgr.active_screen();
    return s ? s->screen_id().c_str() : "";
}

// --- Server List ---

int ao_server_count() {
    auto* sl = as_server_list();
    return sl ? static_cast<int>(sl->get_servers().size()) : 0;
}

const char* ao_server_name(int index) {
    auto* sl = as_server_list();
    if (!sl || index < 0 || index >= (int)sl->get_servers().size())
        return "";
    return sl->get_servers()[index].name.c_str();
}

const char* ao_server_description(int index) {
    auto* sl = as_server_list();
    if (!sl || index < 0 || index >= (int)sl->get_servers().size())
        return "";
    return sl->get_servers()[index].description.c_str();
}

int ao_server_players(int index) {
    auto* sl = as_server_list();
    if (!sl || index < 0 || index >= (int)sl->get_servers().size())
        return 0;
    return sl->get_servers()[index].players;
}

bool ao_server_has_ws(int index) {
    auto* sl = as_server_list();
    if (!sl || index < 0 || index >= (int)sl->get_servers().size())
        return false;
    const auto& s = sl->get_servers()[index];
    return s.ws_port.has_value() || s.wss_port.has_value();
}

int ao_server_selected() {
    auto* sl = as_server_list();
    return sl ? sl->get_selected() : -1;
}

void ao_server_select(int index) {
    auto* sl = as_server_list();
    if (sl)
        sl->select_server(index);
}

void ao_server_direct_connect(const char* host, uint16_t port) {
    auto* sl = as_server_list();
    if (sl && host)
        sl->direct_connect(host, port);
}

// --- Character Select ---

int ao_char_count() {
    auto* cs = as_char_select();
    return cs ? static_cast<int>(cs->get_chars().size()) : 0;
}

const char* ao_char_folder(int index) {
    auto* cs = as_char_select();
    if (!cs || index < 0 || index >= (int)cs->get_chars().size())
        return "";
    return cs->get_chars()[index].folder.c_str();
}

bool ao_char_taken(int index) {
    auto* cs = as_char_select();
    if (!cs || index < 0 || index >= (int)cs->get_chars().size())
        return false;
    return cs->get_chars()[index].taken;
}

bool ao_char_icon_ready(int index) {
    auto* cs = as_char_select();
    if (!cs || index < 0 || index >= (int)cs->get_chars().size())
        return false;
    return cs->get_chars()[index].icon_asset != nullptr;
}

int ao_char_icon_width(int index) {
    auto* cs = as_char_select();
    if (!cs || index < 0 || index >= (int)cs->get_chars().size())
        return 0;
    const auto& asset = cs->get_chars()[index].icon_asset;
    return asset ? asset->width() : 0;
}

int ao_char_icon_height(int index) {
    auto* cs = as_char_select();
    if (!cs || index < 0 || index >= (int)cs->get_chars().size())
        return 0;
    const auto& asset = cs->get_chars()[index].icon_asset;
    return asset ? asset->height() : 0;
}

const uint8_t* ao_char_icon_pixels(int index) {
    auto* cs = as_char_select();
    if (!cs || index < 0 || index >= (int)cs->get_chars().size())
        return nullptr;
    const auto& asset = cs->get_chars()[index].icon_asset;
    return (asset && asset->frame_count() > 0) ? asset->frame_pixels(0) : nullptr;
}

int ao_char_selected() {
    auto* cs = as_char_select();
    return cs ? cs->get_selected() : -1;
}

void ao_char_select(int index) {
    auto* cs = as_char_select();
    if (cs)
        cs->select_character(index);
}

// --- Courtroom ---

const char* ao_courtroom_character() {
    auto* cr = as_courtroom();
    return cr ? cr->get_character_name().c_str() : "";
}

int ao_courtroom_char_id() {
    auto* cr = as_courtroom();
    return cr ? cr->get_char_id() : -1;
}

bool ao_courtroom_loading() {
    auto* cr = as_courtroom();
    return cr ? cr->is_loading() : false;
}

int ao_courtroom_emote_count() {
    auto* cr = as_courtroom();
    if (!cr || !cr->get_character_sheet())
        return 0;
    return cr->get_character_sheet()->emote_count();
}

const char* ao_courtroom_emote_comment(int index) {
    auto* cr = as_courtroom();
    if (!cr || cr->is_loading() || !cr->get_character_sheet())
        return "";
    if (index < 0 || index >= cr->get_character_sheet()->emote_count())
        return "";
    // emote() returns by value — the temporary is destroyed at the semicolon.
    // Copy into a scratch buffer so the pointer survives until Dart reads it.
    g_scratch = cr->get_character_sheet()->emote(index).comment;
    return g_scratch.c_str();
}

bool ao_courtroom_emote_icon_ready(int index) {
    if (index < 0 || index >= (int)g_emote_icon_cache.size())
        return false;
    return g_emote_icon_cache[index] && g_emote_icon_cache[index]->frame_count() > 0;
}

int ao_courtroom_emote_icon_width(int index) {
    if (index < 0 || index >= (int)g_emote_icon_cache.size() || !g_emote_icon_cache[index])
        return 0;
    return g_emote_icon_cache[index]->width();
}

int ao_courtroom_emote_icon_height(int index) {
    if (index < 0 || index >= (int)g_emote_icon_cache.size() || !g_emote_icon_cache[index])
        return 0;
    return g_emote_icon_cache[index]->height();
}

const uint8_t* ao_courtroom_emote_icon_pixels(int index) {
    if (index < 0 || index >= (int)g_emote_icon_cache.size() || !g_emote_icon_cache[index] ||
        g_emote_icon_cache[index]->frame_count() == 0)
        return nullptr;
    return g_emote_icon_cache[index]->frame_pixels(0);
}

// --- IC Chat (send) ---

void ao_ic_send(const char* message) {
    auto* cr = as_courtroom();
    if (!cr || !message)
        return;

    ICMessageData data;
    data.character = cr->get_character_name();
    data.char_id = cr->get_char_id();
    data.message = message;
    data.showname = g_ic_state.showname;

    auto sheet = cr->get_character_sheet();
    if (sheet && g_ic_state.selected_emote >= 0 && g_ic_state.selected_emote < sheet->emote_count()) {
        const auto& emo = sheet->emote(g_ic_state.selected_emote);
        data.emote = emo.anim_name;
        data.pre_emote = emo.pre_anim;
        data.desk_mod = emo.desk_mod;
        if (!emo.sfx_name.empty() && emo.sfx_name != "0") {
            data.sfx_name = emo.sfx_name;
            data.sfx_delay = emo.sfx_delay;
        }
    }

    data.emote_mod = g_ic_state.pre_anim ? 1 : 0;
    data.side = SIDES[std::clamp(g_ic_state.side_index, 0, 6)];
    data.objection_mod = g_ic_state.objection_mod;
    data.flip = g_ic_state.flip ? 1 : 0;
    data.text_color = g_ic_state.text_color;

    EventManager::instance().get_channel<OutgoingICMessageEvent>().publish(OutgoingICMessageEvent(std::move(data)));

    // Reset one-shot state
    g_ic_state.objection_mod = 0;
}

void ao_ic_set_emote(int index) {
    g_ic_state.selected_emote = index;
}

void ao_ic_set_side(int index) {
    g_ic_state.side_index = std::clamp(index, 0, 6);
}

void ao_ic_set_showname(const char* name) {
    g_ic_state.showname = name ? name : "";
}

void ao_ic_set_pre(bool enabled) {
    g_ic_state.pre_anim = enabled;
}

void ao_ic_set_flip(bool enabled) {
    g_ic_state.flip = enabled;
}

void ao_ic_set_interjection(int type) {
    g_ic_state.objection_mod = type;
}

void ao_ic_set_color(int color) {
    g_ic_state.text_color = color;
}

const char* ao_ic_get_showname() {
    return g_ic_state.showname.c_str();
}

int ao_ic_get_side() {
    return g_ic_state.side_index;
}

// --- OOC Chat ---

void ao_ooc_send(const char* name, const char* message) {
    if (!name || !message)
        return;
    EventManager::instance().get_channel<OutgoingChatEvent>().publish(
        OutgoingChatEvent(std::string(name), std::string(message)));
}

int ao_ooc_message_count() {
    return static_cast<int>(g_ooc_msgs.size());
}

const char* ao_ooc_message_name(int index) {
    if (index < 0 || index >= (int)g_ooc_msgs.size())
        return "";
    return g_ooc_msgs[index].name.c_str();
}

const char* ao_ooc_message_text(int index) {
    if (index < 0 || index >= (int)g_ooc_msgs.size())
        return "";
    return g_ooc_msgs[index].text.c_str();
}

void ao_ooc_messages_consume() {
    g_ooc_msgs.clear();
}

// --- IC Log ---

int ao_ic_log_count() {
    return static_cast<int>(g_ic_log.size());
}

const char* ao_ic_log_showname(int index) {
    if (index < 0 || index >= (int)g_ic_log.size())
        return "";
    return g_ic_log[index].name.c_str();
}

const char* ao_ic_log_message(int index) {
    if (index < 0 || index >= (int)g_ic_log.size())
        return "";
    return g_ic_log[index].text.c_str();
}

void ao_ic_log_consume() {
    g_ic_log.clear();
}

// --- Music & Areas ---

int ao_music_count() {
    return static_cast<int>(g_tracks.size());
}

const char* ao_music_name(int index) {
    if (index < 0 || index >= (int)g_tracks.size())
        return "";
    return g_tracks[index].c_str();
}

void ao_music_play(int index) {
    if (index < 0 || index >= (int)g_tracks.size())
        return;
    EventManager::instance().get_channel<OutgoingMusicEvent>().publish(OutgoingMusicEvent(g_tracks[index]));
}

void ao_music_play_by_name(const char* name) {
    if (!name)
        return;
    EventManager::instance().get_channel<OutgoingMusicEvent>().publish(OutgoingMusicEvent(std::string(name)));
}

int ao_area_count() {
    return static_cast<int>(g_areas.size());
}

const char* ao_area_name(int index) {
    if (index < 0 || index >= (int)g_areas.size())
        return "";
    return g_areas[index].c_str();
}

int ao_area_players(int index) {
    if (index < 0 || index >= (int)g_area_players.size())
        return -1;
    return g_area_players[index];
}

const char* ao_area_status(int index) {
    if (index < 0 || index >= (int)g_area_status.size())
        return "";
    return g_area_status[index].c_str();
}

const char* ao_area_cm(int index) {
    if (index < 0 || index >= (int)g_area_cm.size())
        return "";
    return g_area_cm[index].c_str();
}

const char* ao_area_lock(int index) {
    if (index < 0 || index >= (int)g_area_lock.size())
        return "";
    return g_area_lock[index].c_str();
}

const char* ao_now_playing() {
    return g_now_playing.c_str();
}

// --- Disconnect ---

bool ao_disconnect_pending() {
    return g_disconnect_pending;
}

const char* ao_disconnect_reason() {
    return g_disconnect_reason.c_str();
}

void ao_disconnect_consume() {
    g_disconnect_pending = false;
    g_disconnect_reason.clear();
}

// --- Player List ---

int ao_player_count() {
    return static_cast<int>(g_player_ids_cache.size());
}

int ao_player_id(int index) {
    if (index < 0 || index >= (int)g_player_ids_cache.size())
        return -1;
    return g_player_ids_cache[index];
}

const char* ao_player_name(int index) {
    if (index < 0 || index >= (int)g_player_ids_cache.size())
        return "";
    auto it = g_players.find(g_player_ids_cache[index]);
    return (it != g_players.end()) ? it->second.name.c_str() : "";
}

const char* ao_player_character(int index) {
    if (index < 0 || index >= (int)g_player_ids_cache.size())
        return "";
    auto it = g_players.find(g_player_ids_cache[index]);
    return (it != g_players.end()) ? it->second.character.c_str() : "";
}

const char* ao_player_charname(int index) {
    if (index < 0 || index >= (int)g_player_ids_cache.size())
        return "";
    auto it = g_players.find(g_player_ids_cache[index]);
    return (it != g_players.end()) ? it->second.charname.c_str() : "";
}

int ao_player_area(int index) {
    if (index < 0 || index >= (int)g_player_ids_cache.size())
        return -1;
    auto it = g_players.find(g_player_ids_cache[index]);
    return (it != g_players.end()) ? it->second.area_id : -1;
}

// --- Evidence ---

int ao_evidence_count() {
    return static_cast<int>(g_evidence.size());
}

const char* ao_evidence_name(int index) {
    if (index < 0 || index >= (int)g_evidence.size())
        return "";
    return g_evidence[index].name.c_str();
}

const char* ao_evidence_description(int index) {
    if (index < 0 || index >= (int)g_evidence.size())
        return "";
    return g_evidence[index].description.c_str();
}

const char* ao_evidence_image(int index) {
    if (index < 0 || index >= (int)g_evidence.size())
        return "";
    return g_evidence[index].image.c_str();
}

// --- Health Bars ---

int ao_hp_defense() {
    return g_def_hp;
}

int ao_hp_prosecution() {
    return g_pro_hp;
}

void ao_hp_set(int side, int value) {
    EventManager::instance().get_channel<OutgoingHealthBarEvent>().publish(
        OutgoingHealthBarEvent(side, std::clamp(value, 0, 10)));
}

// --- Timers ---

int ao_timer_count() {
    return MAX_TIMERS;
}

bool ao_timer_visible(int index) {
    if (index < 0 || index >= MAX_TIMERS)
        return false;
    return g_timers[index].visible;
}

bool ao_timer_running(int index) {
    if (index < 0 || index >= MAX_TIMERS)
        return false;
    return g_timers[index].running;
}

int64_t ao_timer_remaining_ms(int index) {
    if (index < 0 || index >= MAX_TIMERS)
        return 0;
    return g_timers[index].remaining_ms;
}

// --- Server Info ---

const char* ao_server_info_software() {
    return g_server_software.c_str();
}

const char* ao_server_info_version() {
    return g_server_version.c_str();
}

int ao_server_info_player_num() {
    return g_server_player_num;
}

// --- Player Count ---

int ao_player_count_current() {
    return g_pcount_current;
}

int ao_player_count_max() {
    return g_pcount_max;
}

const char* ao_player_count_description() {
    return g_pcount_description.c_str();
}

// --- Volume ---

void ao_volume_set(int category, float volume) {
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
    case 3:
        cat = VolumeChangeEvent::Category::MASTER;
        break;
    default:
        return;
    }
    EventManager::instance().get_channel<VolumeChangeEvent>().publish(VolumeChangeEvent(cat, volume));
}

// --- Navigation ---

void ao_nav_pop() {
    if (g_engine)
        g_engine->ui_mgr.pop_screen();
}

void ao_nav_pop_to_root() {
    if (g_engine)
        g_engine->ui_mgr.pop_to_root();
}

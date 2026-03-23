/**
 * @file bridge.h
 * @brief C API bridge between AO-SDL engine and Flutter/Dart via FFI.
 *
 * This is the sole interface between the Dart UI layer and the C++ engine.
 * All functions use C linkage and plain types so dart:ffi can call them
 * directly. The bridge mirrors the role of apps/sdl/main.cpp — it wires
 * together the engine, plugins, and renderer, then exposes handles for
 * Flutter to drive the frame loop and read screen state.
 */
#pragma once

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

// ---------------------------------------------------------------------------
// Lifecycle
// ---------------------------------------------------------------------------

/// Initialize the engine: mount manager, HTTP pool, asset library, event
/// system, and network thread. Call once at app startup.
/// @param base_path  Path to the "base" asset directory (may be NULL).
void ao_init(const char* base_path);

/// Shut down all threads and release engine resources. Call on app exit.
void ao_shutdown(void);

// ---------------------------------------------------------------------------
// Frame pump — called from Flutter's frame callback
// ---------------------------------------------------------------------------

/// Pump the main-thread work: poll HTTP, handle asset URL events, and
/// advance the UI manager (handle_events on active screen).
void ao_tick(void);

// ---------------------------------------------------------------------------
// Renderer — offscreen scene texture
// ---------------------------------------------------------------------------

/// Create the offscreen renderer at the given resolution.
/// @param width  Framebuffer width in pixels.
/// @param height Framebuffer height in pixels.
/// @param use_metal  True to use Metal (iOS), false for GLES (Android).
void ao_renderer_create(int width, int height, bool use_metal);

/// Draw one frame of the courtroom scene to the offscreen texture.
void ao_renderer_draw(void);

/// Get the offscreen render texture handle.
/// On Metal: id<MTLTexture> cast to uintptr_t.
/// On GLES: GLuint cast to uintptr_t.
uintptr_t ao_renderer_get_texture(void);

/// Resize the offscreen render target.
void ao_renderer_resize(int width, int height);

/// Get the Metal device pointer (iOS only, NULL on Android).
void* ao_renderer_get_metal_device(void);

/// Get the Metal command queue pointer (iOS only, NULL on Android).
void* ao_renderer_get_metal_command_queue(void);

// ---------------------------------------------------------------------------
// Screen state — read-only queries for Flutter UI
// ---------------------------------------------------------------------------

/// Get the screen_id() of the active screen (e.g. "server_list").
/// Returns a pointer to a static string — valid until the next screen change.
const char* ao_active_screen_id(void);

// --- Server List ---

/// Number of servers in the list (0 if not yet fetched).
int ao_server_count(void);

/// Get server name at index. Returns pointer valid until next server list update.
const char* ao_server_name(int index);

/// Get server description at index.
const char* ao_server_description(int index);

/// Get server player count at index.
int ao_server_players(int index);

/// Whether the server at index supports WebSocket (i.e. is joinable).
bool ao_server_has_ws(int index);

/// Get the currently selected server index (-1 if none).
int ao_server_selected(void);

/// Select and connect to a server by index.
void ao_server_select(int index);

/// Direct connect to host:port.
void ao_server_direct_connect(const char* host, uint16_t port);

// --- Character Select ---

/// Number of characters in the roster.
int ao_char_count(void);

/// Get character folder name at index.
const char* ao_char_folder(int index);

/// Whether the character at index is taken by another player.
bool ao_char_taken(int index);

/// Whether the character icon at index has been loaded.
bool ao_char_icon_ready(int index);

/// Get character icon width (0 if not loaded).
int ao_char_icon_width(int index);

/// Get character icon height (0 if not loaded).
int ao_char_icon_height(int index);

/// Get pointer to character icon RGBA pixel data (NULL if not loaded).
/// The pointer is valid as long as the character select screen is active.
const uint8_t* ao_char_icon_pixels(int index);

/// Get the currently selected character index (-1 if none).
int ao_char_selected(void);

/// Select a character by index.
void ao_char_select(int index);

// --- Courtroom ---

/// Get the current character name in the courtroom.
const char* ao_courtroom_character(void);

/// Get the character ID in the courtroom.
int ao_courtroom_char_id(void);

/// Whether the courtroom is still loading character data.
bool ao_courtroom_loading(void);

/// Number of emotes available for the current character.
int ao_courtroom_emote_count(void);

/// Get emote comment/label at index.
const char* ao_courtroom_emote_comment(int index);

/// Whether the emote icon at index has been loaded.
bool ao_courtroom_emote_icon_ready(int index);

/// Get emote icon width (0 if not loaded).
int ao_courtroom_emote_icon_width(int index);

/// Get emote icon height (0 if not loaded).
int ao_courtroom_emote_icon_height(int index);

/// Get pointer to emote icon RGBA pixel data (NULL if not loaded).
const uint8_t* ao_courtroom_emote_icon_pixels(int index);

// --- IC Chat (send messages) ---

/// Send an IC message with the given text and current emote/settings.
void ao_ic_send(const char* message);

/// Set the selected emote index.
void ao_ic_set_emote(int index);

/// Set the side/position index (0=def, 1=pro, 2=wit, etc.).
void ao_ic_set_side(int index);

/// Set the showname text.
void ao_ic_set_showname(const char* name);

/// Set pre-animation enabled/disabled.
void ao_ic_set_pre(bool enabled);

/// Set flip enabled/disabled.
void ao_ic_set_flip(bool enabled);

/// Set the interjection type (0=none, 1=holdit, 2=objection, 3=takethat).
void ao_ic_set_interjection(int type);

/// Set the text color index.
void ao_ic_set_color(int color);

/// Get the current showname (may be auto-populated from char.ini).
const char* ao_ic_get_showname(void);

/// Get the current side index (may be auto-populated from char.ini).
int ao_ic_get_side(void);

// --- OOC Chat ---

/// Send an OOC (out-of-character) message.
void ao_ooc_send(const char* name, const char* message);

/// Get the number of pending OOC messages to display.
int ao_ooc_message_count(void);

/// Get OOC message sender at index.
const char* ao_ooc_message_name(int index);

/// Get OOC message text at index.
const char* ao_ooc_message_text(int index);

/// Consume (clear) all OOC messages after reading them.
void ao_ooc_messages_consume(void);

// --- IC Log ---

/// Get the number of IC log entries.
int ao_ic_log_count(void);

/// Get IC log entry showname at index.
const char* ao_ic_log_showname(int index);

/// Get IC log entry message text at index.
const char* ao_ic_log_message(int index);

/// Consume (clear) all IC log entries after reading them.
void ao_ic_log_consume(void);

// --- Music & Areas ---

/// Number of music tracks available.
int ao_music_count(void);

/// Get music track name at index.
const char* ao_music_name(int index);

/// Play a music track by name (publishes OutgoingMusicEvent).
void ao_music_play(int index);

/// Play a music track or switch area by name (publishes OutgoingMusicEvent).
void ao_music_play_by_name(const char* name);

/// Number of areas available.
int ao_area_count(void);

/// Get area name at index.
const char* ao_area_name(int index);

/// Get area player count at index (-1 if unknown).
int ao_area_players(int index);

/// Get area status string at index.
const char* ao_area_status(int index);

/// Get area CM string at index.
const char* ao_area_cm(int index);

/// Get area lock status string at index.
const char* ao_area_lock(int index);

/// Get the currently playing music track name.
const char* ao_now_playing(void);

// --- Disconnect ---

/// Whether a disconnect event has occurred since last check.
bool ao_disconnect_pending(void);

/// Get the disconnect reason string.
const char* ao_disconnect_reason(void);

/// Clear the disconnect state after handling it.
void ao_disconnect_consume(void);

// --- Player List ---

/// Number of players online.
int ao_player_count(void);

/// Get player ID at list index.
int ao_player_id(int index);

/// Get player display name at list index.
const char* ao_player_name(int index);

/// Get player character name at list index.
const char* ao_player_character(int index);

/// Get player charname (showname) at list index.
const char* ao_player_charname(int index);

/// Get player area ID at list index (-1 if unknown).
int ao_player_area(int index);

// --- Evidence ---

/// Number of evidence items.
int ao_evidence_count(void);

/// Get evidence name at index.
const char* ao_evidence_name(int index);

/// Get evidence description at index.
const char* ao_evidence_description(int index);

/// Get evidence image filename at index.
const char* ao_evidence_image(int index);

// --- Health Bars ---

/// Get defense HP (0-10).
int ao_hp_defense(void);

/// Get prosecution HP (0-10).
int ao_hp_prosecution(void);

/// Set a health bar value (publishes OutgoingHealthBarEvent).
/// @param side  1 = defense, 2 = prosecution.
/// @param value 0-10.
void ao_hp_set(int side, int value);

// --- Timers ---

/// Get the number of timer slots (fixed at 4).
int ao_timer_count(void);

/// Whether timer at index is visible.
bool ao_timer_visible(int index);

/// Whether timer at index is running.
bool ao_timer_running(int index);

/// Get timer remaining time in milliseconds.
int64_t ao_timer_remaining_ms(int index);

// --- Server Info ---

/// Get server software name (empty if not yet received).
const char* ao_server_info_software(void);

/// Get server version string.
const char* ao_server_info_version(void);

/// Get assigned player number.
int ao_server_info_player_num(void);

// --- Player Count ---

/// Get current player count on connected server.
int ao_player_count_current(void);

/// Get max player count on connected server.
int ao_player_count_max(void);

/// Get server description from PN packet.
const char* ao_player_count_description(void);

// --- Volume ---

/// Set volume for a category (0=music, 1=sfx, 2=blip, 3=master).
/// @param category Volume category index.
/// @param volume   0.0 to 1.0 amplitude.
void ao_volume_set(int category, float volume);

// --- Navigation ---

/// Pop the current screen (e.g. courtroom → char select).
void ao_nav_pop(void);

/// Pop to root (e.g. courtroom → server list, disconnects).
void ao_nav_pop_to_root(void);

#ifdef __cplusplus
}
#endif

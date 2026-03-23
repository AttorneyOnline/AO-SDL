/// FFI bindings to the AO-SDL engine via native/bridge.h.
///
/// This is the Dart equivalent of the C bridge — every function here
/// maps 1:1 to a C function in bridge.h via dart:ffi.
library;

import 'dart:convert';
import 'dart:ffi';
import 'dart:io' show Platform;
import 'dart:typed_data';

import 'package:ffi/ffi.dart';

/// Safely convert a native C string to Dart.
/// Tries UTF-8 first; falls back to Latin-1 (ISO 8859-1) for char.ini
/// strings that use Windows-1252/Latin-1 encoding.
String _safeString(Pointer<Utf8> ptr) {
  if (ptr == nullptr || ptr.address == 0) return '';
  try {
    final charPtr = ptr.cast<Uint8>();
    var len = 0;
    while (len < 4096 && charPtr[len] != 0) {
      len++;
    }
    if (len == 0) return '';
    final bytes = Uint8List(len);
    for (var i = 0; i < len; i++) {
      bytes[i] = charPtr[i];
    }
    // Try UTF-8 first; if it fails, treat as Latin-1
    try {
      return utf8.decode(bytes);
    } on FormatException {
      return latin1.decode(bytes);
    }
  } catch (_) {
    return '';
  }
}

// ---------------------------------------------------------------------------
// Load the native library
// ---------------------------------------------------------------------------

final DynamicLibrary _lib = _loadLibrary();

DynamicLibrary _loadLibrary() {
  if (Platform.isIOS) {
    // On iOS, the bridge is statically linked into the runner.
    return DynamicLibrary.process();
  }
  if (Platform.isAndroid) {
    return DynamicLibrary.open('libao_bridge.so');
  }
  // Desktop fallback for testing
  if (Platform.isMacOS) {
    return DynamicLibrary.open('libao_bridge.dylib');
  }
  throw UnsupportedError('Unsupported platform: ${Platform.operatingSystem}');
}

// ---------------------------------------------------------------------------
// Type aliases for native function signatures
// ---------------------------------------------------------------------------

// void ao_init(const char*)
typedef _InitNative = Void Function(Pointer<Utf8>);
typedef _InitDart = void Function(Pointer<Utf8>);

// void ao_shutdown()
typedef _VoidNative = Void Function();
typedef _VoidDart = void Function();

// void ao_tick()
// (same as _VoidNative / _VoidDart)

// void ao_renderer_create(int, int, bool)
typedef _RendererCreateNative = Void Function(Int32, Int32, Bool);
typedef _RendererCreateDart = void Function(int, int, bool);

// uintptr_t ao_renderer_get_texture()
typedef _GetTextureNative = UintPtr Function();
typedef _GetTextureDart = int Function();

// void ao_renderer_resize(int, int)
typedef _ResizeNative = Void Function(Int32, Int32);
typedef _ResizeDart = void Function(int, int);

// void* ao_renderer_get_metal_device()
typedef _GetPtrNative = Pointer<Void> Function();
typedef _GetPtrDart = Pointer<Void> Function();

// const char* ao_active_screen_id()
typedef _GetStringNative = Pointer<Utf8> Function();
typedef _GetStringDart = Pointer<Utf8> Function();

// int ao_server_count()
typedef _GetIntNative = Int32 Function();
typedef _GetIntDart = int Function();

// const char* ao_server_name(int)
typedef _GetStringByIdxNative = Pointer<Utf8> Function(Int32);
typedef _GetStringByIdxDart = Pointer<Utf8> Function(int);

// int ao_server_players(int)
typedef _GetIntByIdxNative = Int32 Function(Int32);
typedef _GetIntByIdxDart = int Function(int);

// bool ao_server_has_ws(int)
typedef _GetBoolByIdxNative = Bool Function(Int32);
typedef _GetBoolByIdxDart = bool Function(int);

// void ao_server_select(int)
typedef _SetIntNative = Void Function(Int32);
typedef _SetIntDart = void Function(int);

// void ao_server_direct_connect(const char*, uint16_t)
typedef _DirectConnectNative = Void Function(Pointer<Utf8>, Uint16);
typedef _DirectConnectDart = void Function(Pointer<Utf8>, int);

// void ao_ic_send(const char*)
typedef _SendStringNative = Void Function(Pointer<Utf8>);
typedef _SendStringDart = void Function(Pointer<Utf8>);

// void ao_ic_set_pre(bool)
typedef _SetBoolNative = Void Function(Bool);
typedef _SetBoolDart = void Function(bool);

// void ao_ooc_send(const char*, const char*)
typedef _OocSendNative = Void Function(Pointer<Utf8>, Pointer<Utf8>);
typedef _OocSendDart = void Function(Pointer<Utf8>, Pointer<Utf8>);

// ---------------------------------------------------------------------------
// Bound functions
// ---------------------------------------------------------------------------

final _init = _lib.lookupFunction<_InitNative, _InitDart>('ao_init');
final _shutdown = _lib.lookupFunction<_VoidNative, _VoidDart>('ao_shutdown');
final _tick = _lib.lookupFunction<_VoidNative, _VoidDart>('ao_tick');

final _rendererCreate = _lib.lookupFunction<_RendererCreateNative, _RendererCreateDart>('ao_renderer_create');
final _rendererDraw = _lib.lookupFunction<_VoidNative, _VoidDart>('ao_renderer_draw');
final _rendererGetTexture = _lib.lookupFunction<_GetTextureNative, _GetTextureDart>('ao_renderer_get_texture');
final _rendererResize = _lib.lookupFunction<_ResizeNative, _ResizeDart>('ao_renderer_resize');
// ignore: unused_element
final _rendererGetMetalDevice = _lib.lookupFunction<_GetPtrNative, _GetPtrDart>('ao_renderer_get_metal_device');
// ignore: unused_element
final _rendererGetMetalCommandQueue = _lib.lookupFunction<_GetPtrNative, _GetPtrDart>('ao_renderer_get_metal_command_queue');

final _activeScreenId = _lib.lookupFunction<_GetStringNative, _GetStringDart>('ao_active_screen_id');

// Server list
final _serverCount = _lib.lookupFunction<_GetIntNative, _GetIntDart>('ao_server_count');
final _serverName = _lib.lookupFunction<_GetStringByIdxNative, _GetStringByIdxDart>('ao_server_name');
final _serverDescription = _lib.lookupFunction<_GetStringByIdxNative, _GetStringByIdxDart>('ao_server_description');
final _serverPlayers = _lib.lookupFunction<_GetIntByIdxNative, _GetIntByIdxDart>('ao_server_players');
final _serverHasWs = _lib.lookupFunction<_GetBoolByIdxNative, _GetBoolByIdxDart>('ao_server_has_ws');
final _serverSelected = _lib.lookupFunction<_GetIntNative, _GetIntDart>('ao_server_selected');
final _serverSelect = _lib.lookupFunction<_SetIntNative, _SetIntDart>('ao_server_select');
final _serverDirectConnect = _lib.lookupFunction<_DirectConnectNative, _DirectConnectDart>('ao_server_direct_connect');

// Character select
final _charCount = _lib.lookupFunction<_GetIntNative, _GetIntDart>('ao_char_count');
final _charFolder = _lib.lookupFunction<_GetStringByIdxNative, _GetStringByIdxDart>('ao_char_folder');
final _charTaken = _lib.lookupFunction<_GetBoolByIdxNative, _GetBoolByIdxDart>('ao_char_taken');
final _charIconReady = _lib.lookupFunction<_GetBoolByIdxNative, _GetBoolByIdxDart>('ao_char_icon_ready');
final _charIconWidth = _lib.lookupFunction<_GetIntByIdxNative, _GetIntByIdxDart>('ao_char_icon_width');
final _charIconHeight = _lib.lookupFunction<_GetIntByIdxNative, _GetIntByIdxDart>('ao_char_icon_height');
final _charIconPixels = _lib.lookupFunction<Pointer<Uint8> Function(Int32), Pointer<Uint8> Function(int)>('ao_char_icon_pixels');
final _charSelected = _lib.lookupFunction<_GetIntNative, _GetIntDart>('ao_char_selected');
final _charSelect = _lib.lookupFunction<_SetIntNative, _SetIntDart>('ao_char_select');

// Courtroom
final _courtroomCharacter = _lib.lookupFunction<_GetStringNative, _GetStringDart>('ao_courtroom_character');
final _courtroomCharId = _lib.lookupFunction<_GetIntNative, _GetIntDart>('ao_courtroom_char_id');
final _courtroomLoading = _lib.lookupFunction<Bool Function(), bool Function()>('ao_courtroom_loading');
final _courtroomEmoteCount = _lib.lookupFunction<_GetIntNative, _GetIntDart>('ao_courtroom_emote_count');
final _courtroomEmoteComment = _lib.lookupFunction<_GetStringByIdxNative, _GetStringByIdxDart>('ao_courtroom_emote_comment');
final _courtroomEmoteIconReady = _lib.lookupFunction<_GetBoolByIdxNative, _GetBoolByIdxDart>('ao_courtroom_emote_icon_ready');
final _courtroomEmoteIconWidth = _lib.lookupFunction<_GetIntByIdxNative, _GetIntByIdxDart>('ao_courtroom_emote_icon_width');
final _courtroomEmoteIconHeight = _lib.lookupFunction<_GetIntByIdxNative, _GetIntByIdxDart>('ao_courtroom_emote_icon_height');
final _courtroomEmoteIconPixels = _lib.lookupFunction<Pointer<Uint8> Function(Int32), Pointer<Uint8> Function(int)>('ao_courtroom_emote_icon_pixels');

// IC chat
final _icSend = _lib.lookupFunction<_SendStringNative, _SendStringDart>('ao_ic_send');
final _icSetEmote = _lib.lookupFunction<_SetIntNative, _SetIntDart>('ao_ic_set_emote');
final _icSetSide = _lib.lookupFunction<_SetIntNative, _SetIntDart>('ao_ic_set_side');
final _icSetShowname = _lib.lookupFunction<_SendStringNative, _SendStringDart>('ao_ic_set_showname');
final _icSetPre = _lib.lookupFunction<_SetBoolNative, _SetBoolDart>('ao_ic_set_pre');
final _icSetFlip = _lib.lookupFunction<_SetBoolNative, _SetBoolDart>('ao_ic_set_flip');
final _icSetInterjection = _lib.lookupFunction<_SetIntNative, _SetIntDart>('ao_ic_set_interjection');
final _icSetColor = _lib.lookupFunction<_SetIntNative, _SetIntDart>('ao_ic_set_color');
final _icGetShowname = _lib.lookupFunction<_GetStringNative, _GetStringDart>('ao_ic_get_showname');
final _icGetSide = _lib.lookupFunction<_GetIntNative, _GetIntDart>('ao_ic_get_side');

// OOC chat
final _oocSend = _lib.lookupFunction<_OocSendNative, _OocSendDart>('ao_ooc_send');
final _oocMessageCount = _lib.lookupFunction<_GetIntNative, _GetIntDart>('ao_ooc_message_count');
final _oocMessageName = _lib.lookupFunction<_GetStringByIdxNative, _GetStringByIdxDart>('ao_ooc_message_name');
final _oocMessageText = _lib.lookupFunction<_GetStringByIdxNative, _GetStringByIdxDart>('ao_ooc_message_text');
final _oocMessagesConsume = _lib.lookupFunction<_VoidNative, _VoidDart>('ao_ooc_messages_consume');

// IC log
final _icLogCount = _lib.lookupFunction<_GetIntNative, _GetIntDart>('ao_ic_log_count');
final _icLogShowname = _lib.lookupFunction<_GetStringByIdxNative, _GetStringByIdxDart>('ao_ic_log_showname');
final _icLogMessage = _lib.lookupFunction<_GetStringByIdxNative, _GetStringByIdxDart>('ao_ic_log_message');
final _icLogConsume = _lib.lookupFunction<_VoidNative, _VoidDart>('ao_ic_log_consume');

// Music & Areas
final _musicCount = _lib.lookupFunction<_GetIntNative, _GetIntDart>('ao_music_count');
final _musicName = _lib.lookupFunction<_GetStringByIdxNative, _GetStringByIdxDart>('ao_music_name');
final _musicPlay = _lib.lookupFunction<_SetIntNative, _SetIntDart>('ao_music_play');
final _musicPlayByName = _lib.lookupFunction<_SendStringNative, _SendStringDart>('ao_music_play_by_name');
final _areaCount = _lib.lookupFunction<_GetIntNative, _GetIntDart>('ao_area_count');
final _areaName = _lib.lookupFunction<_GetStringByIdxNative, _GetStringByIdxDart>('ao_area_name');
final _areaPlayers = _lib.lookupFunction<_GetIntByIdxNative, _GetIntByIdxDart>('ao_area_players');
final _areaStatus = _lib.lookupFunction<_GetStringByIdxNative, _GetStringByIdxDart>('ao_area_status');
final _areaCm = _lib.lookupFunction<_GetStringByIdxNative, _GetStringByIdxDart>('ao_area_cm');
final _areaLock = _lib.lookupFunction<_GetStringByIdxNative, _GetStringByIdxDart>('ao_area_lock');
final _nowPlaying = _lib.lookupFunction<_GetStringNative, _GetStringDart>('ao_now_playing');

// Disconnect
final _disconnectPending = _lib.lookupFunction<Bool Function(), bool Function()>('ao_disconnect_pending');
final _disconnectReason = _lib.lookupFunction<_GetStringNative, _GetStringDart>('ao_disconnect_reason');
final _disconnectConsume = _lib.lookupFunction<_VoidNative, _VoidDart>('ao_disconnect_consume');

// Player list
final _playerCount = _lib.lookupFunction<_GetIntNative, _GetIntDart>('ao_player_count');
final _playerId = _lib.lookupFunction<_GetIntByIdxNative, _GetIntByIdxDart>('ao_player_id');
final _playerName = _lib.lookupFunction<_GetStringByIdxNative, _GetStringByIdxDart>('ao_player_name');
final _playerCharacter = _lib.lookupFunction<_GetStringByIdxNative, _GetStringByIdxDart>('ao_player_character');
final _playerCharname = _lib.lookupFunction<_GetStringByIdxNative, _GetStringByIdxDart>('ao_player_charname');
final _playerArea = _lib.lookupFunction<_GetIntByIdxNative, _GetIntByIdxDart>('ao_player_area');

// Evidence
final _evidenceCount = _lib.lookupFunction<_GetIntNative, _GetIntDart>('ao_evidence_count');
final _evidenceName = _lib.lookupFunction<_GetStringByIdxNative, _GetStringByIdxDart>('ao_evidence_name');
final _evidenceDescription = _lib.lookupFunction<_GetStringByIdxNative, _GetStringByIdxDart>('ao_evidence_description');
final _evidenceImage = _lib.lookupFunction<_GetStringByIdxNative, _GetStringByIdxDart>('ao_evidence_image');

// Health bars
final _hpDefense = _lib.lookupFunction<_GetIntNative, _GetIntDart>('ao_hp_defense');
final _hpProsecution = _lib.lookupFunction<_GetIntNative, _GetIntDart>('ao_hp_prosecution');
final _hpSet = _lib.lookupFunction<Void Function(Int32, Int32), void Function(int, int)>('ao_hp_set');

// Timers
final _timerCount = _lib.lookupFunction<_GetIntNative, _GetIntDart>('ao_timer_count');
final _timerVisible = _lib.lookupFunction<_GetBoolByIdxNative, _GetBoolByIdxDart>('ao_timer_visible');
final _timerRunning = _lib.lookupFunction<_GetBoolByIdxNative, _GetBoolByIdxDart>('ao_timer_running');
final _timerRemainingMs = _lib.lookupFunction<Int64 Function(Int32), int Function(int)>('ao_timer_remaining_ms');

// Server info
final _serverInfoSoftware = _lib.lookupFunction<_GetStringNative, _GetStringDart>('ao_server_info_software');
final _serverInfoVersion = _lib.lookupFunction<_GetStringNative, _GetStringDart>('ao_server_info_version');
final _serverInfoPlayerNum = _lib.lookupFunction<_GetIntNative, _GetIntDart>('ao_server_info_player_num');

// Player count
final _playerCountCurrent = _lib.lookupFunction<_GetIntNative, _GetIntDart>('ao_player_count_current');
final _playerCountMax = _lib.lookupFunction<_GetIntNative, _GetIntDart>('ao_player_count_max');
final _playerCountDescription = _lib.lookupFunction<_GetStringNative, _GetStringDart>('ao_player_count_description');

// Volume
final _volumeSet = _lib.lookupFunction<Void Function(Int32, Float), void Function(int, double)>('ao_volume_set');

// Navigation
final _navPop = _lib.lookupFunction<_VoidNative, _VoidDart>('ao_nav_pop');
final _navPopToRoot = _lib.lookupFunction<_VoidNative, _VoidDart>('ao_nav_pop_to_root');

// ---------------------------------------------------------------------------
// Public Dart API — wraps the raw FFI calls with Dart-friendly types
// ---------------------------------------------------------------------------

class AoBridge {
  static void init(String? basePath) {
    final ptr = basePath != null ? basePath.toNativeUtf8() : nullptr;
    _init(ptr);
    if (ptr != nullptr) calloc.free(ptr);
  }

  static void shutdown() => _shutdown();
  static void tick() => _tick();

  // Renderer
  static void rendererCreate(int w, int h, {bool useMetal = false}) =>
      _rendererCreate(w, h, useMetal);
  static void rendererDraw() => _rendererDraw();
  static int rendererGetTexture() => _rendererGetTexture();
  static void rendererResize(int w, int h) => _rendererResize(w, h);

  // Active screen
  static String activeScreenId() => _safeString(_activeScreenId());

  // Server list
  static int serverCount() => _serverCount();
  static String serverName(int i) => _safeString(_serverName(i));
  static String serverDescription(int i) => _safeString(_serverDescription(i));
  static int serverPlayers(int i) => _serverPlayers(i);
  static bool serverHasWs(int i) => _serverHasWs(i);
  static int serverSelected() => _serverSelected();
  static void serverSelect(int i) => _serverSelect(i);

  static void serverDirectConnect(String host, int port) {
    final h = host.toNativeUtf8();
    _serverDirectConnect(h, port);
    calloc.free(h);
  }

  // Character select
  static int charCount() => _charCount();
  static String charFolder(int i) => _safeString(_charFolder(i));
  static bool charTaken(int i) => _charTaken(i);
  static bool charIconReady(int i) => _charIconReady(i);
  static int charIconWidth(int i) => _charIconWidth(i);
  static int charIconHeight(int i) => _charIconHeight(i);
  static Pointer<Uint8> charIconPixels(int i) => _charIconPixels(i);
  static int charSelected() => _charSelected();
  static void charSelect(int i) => _charSelect(i);

  // Courtroom
  static String courtroomCharacter() => _safeString(_courtroomCharacter());
  static int courtroomCharId() => _courtroomCharId();
  static bool courtroomLoading() => _courtroomLoading();
  static int courtroomEmoteCount() => _courtroomEmoteCount();
  static String courtroomEmoteComment(int i) => _safeString(_courtroomEmoteComment(i));
  static bool courtroomEmoteIconReady(int i) => _courtroomEmoteIconReady(i);
  static int courtroomEmoteIconWidth(int i) => _courtroomEmoteIconWidth(i);
  static int courtroomEmoteIconHeight(int i) => _courtroomEmoteIconHeight(i);
  static Pointer<Uint8> courtroomEmoteIconPixels(int i) => _courtroomEmoteIconPixels(i);

  // IC chat
  static void icSend(String message) {
    final m = message.toNativeUtf8();
    _icSend(m);
    calloc.free(m);
  }

  static void icSetEmote(int i) => _icSetEmote(i);
  static void icSetSide(int i) => _icSetSide(i);

  static void icSetShowname(String name) {
    final n = name.toNativeUtf8();
    _icSetShowname(n);
    calloc.free(n);
  }

  static void icSetPre(bool v) => _icSetPre(v);
  static void icSetFlip(bool v) => _icSetFlip(v);
  static void icSetInterjection(int t) => _icSetInterjection(t);
  static void icSetColor(int c) => _icSetColor(c);
  static String icGetShowname() => _safeString(_icGetShowname());
  static int icGetSide() => _icGetSide();

  // OOC chat
  static void oocSend(String name, String message) {
    final n = name.toNativeUtf8();
    final m = message.toNativeUtf8();
    _oocSend(n, m);
    calloc.free(n);
    calloc.free(m);
  }

  static List<({String name, String text})> oocMessages() {
    final count = _oocMessageCount();
    final msgs = <({String name, String text})>[];
    for (var i = 0; i < count; i++) {
      msgs.add((
        name: _safeString(_oocMessageName(i)),
        text: _safeString(_oocMessageText(i)),
      ));
    }
    _oocMessagesConsume();
    return msgs;
  }

  // IC log
  static List<({String showname, String message})> icLog() {
    final count = _icLogCount();
    final entries = <({String showname, String message})>[];
    for (var i = 0; i < count; i++) {
      entries.add((
        showname: _safeString(_icLogShowname(i)),
        message: _safeString(_icLogMessage(i)),
      ));
    }
    _icLogConsume();
    return entries;
  }

  // Music & Areas
  static int musicCount() => _musicCount();
  static String musicName(int i) => _safeString(_musicName(i));
  static void musicPlay(int i) => _musicPlay(i);

  static void musicPlayByName(String name) {
    final n = name.toNativeUtf8();
    _musicPlayByName(n);
    calloc.free(n);
  }

  static int areaCount() => _areaCount();
  static String areaName(int i) => _safeString(_areaName(i));
  static int areaPlayers(int i) => _areaPlayers(i);
  static String areaStatus(int i) => _safeString(_areaStatus(i));
  static String areaCm(int i) => _safeString(_areaCm(i));
  static String areaLock(int i) => _safeString(_areaLock(i));
  static String nowPlaying() => _safeString(_nowPlaying());

  // Disconnect
  static bool disconnectPending() => _disconnectPending();
  static String disconnectReason() => _safeString(_disconnectReason());
  static void disconnectConsume() => _disconnectConsume();

  // Player list
  static int playerCount() => _playerCount();
  static int playerId(int i) => _playerId(i);
  static String playerName(int i) => _safeString(_playerName(i));
  static String playerCharacter(int i) => _safeString(_playerCharacter(i));
  static String playerCharname(int i) => _safeString(_playerCharname(i));
  static int playerArea(int i) => _playerArea(i);

  // Evidence
  static int evidenceCount() => _evidenceCount();
  static String evidenceName(int i) => _safeString(_evidenceName(i));
  static String evidenceDescription(int i) => _safeString(_evidenceDescription(i));
  static String evidenceImage(int i) => _safeString(_evidenceImage(i));

  // Health bars
  static int hpDefense() => _hpDefense();
  static int hpProsecution() => _hpProsecution();
  static void hpSet(int side, int value) => _hpSet(side, value);

  // Timers
  static int timerCount() => _timerCount();
  static bool timerVisible(int i) => _timerVisible(i);
  static bool timerRunning(int i) => _timerRunning(i);
  static int timerRemainingMs(int i) => _timerRemainingMs(i);

  // Server info
  static String serverInfoSoftware() => _safeString(_serverInfoSoftware());
  static String serverInfoVersion() => _safeString(_serverInfoVersion());
  static int serverInfoPlayerNum() => _serverInfoPlayerNum();

  // Player count
  static int playerCountCurrent() => _playerCountCurrent();
  static int playerCountMax() => _playerCountMax();
  static String playerCountDescription() => _safeString(_playerCountDescription());

  // Volume
  static void volumeSet(int category, double volume) => _volumeSet(category, volume);

  // Navigation
  static void navPop() => _navPop();
  static void navPopToRoot() => _navPopToRoot();
}

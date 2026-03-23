/// Reactive state layer that polls the native bridge and notifies Flutter.
///
/// This is the glue between the C++ engine and Flutter's widget tree.
/// A periodic timer calls ao_tick() and reads the current screen state,
/// then notifies listeners so widgets rebuild.
library;

import 'dart:async';

import 'package:flutter/foundation.dart';

import 'bridge/native_bridge.dart';

/// Top-level engine state exposed to the widget tree via Provider.
class EngineState extends ChangeNotifier {
  Timer? _ticker;
  String _activeScreen = '';
  bool _nativeAvailable = false;

  String get activeScreen => _activeScreen;
  bool get nativeAvailable => _nativeAvailable;

  void start(String? basePath) {
    try {
      AoBridge.init(basePath);
      _nativeAvailable = true;
    } catch (e) {
      debugPrint('Native bridge not available: $e');
      debugPrint('Running in UI-only mode (no engine).');
      _nativeAvailable = false;
      // Default to server_list screen for UI preview
      _activeScreen = 'server_list';
      notifyListeners();
      return;
    }

    // Poll engine at ~30 Hz — matches the game thread's ~10 Hz tick rate
    // with headroom for smooth UI updates.
    _ticker = Timer.periodic(const Duration(milliseconds: 33), (_) {
      _poll();
    });
  }

  void _poll() {
    AoBridge.tick();

    final screen = AoBridge.activeScreenId();
    if (screen != _activeScreen) {
      _activeScreen = screen;
      notifyListeners();
    }

    // Notify on every tick so screen-specific widgets can re-read state
    notifyListeners();
  }

  @override
  void dispose() {
    _ticker?.cancel();
    if (_nativeAvailable) {
      AoBridge.shutdown();
    }
    super.dispose();
  }
}

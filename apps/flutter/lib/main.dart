import 'package:flutter/material.dart';
import 'package:provider/provider.dart';

import 'engine_state.dart';
import 'screens/server_list_screen.dart';
import 'screens/char_select_screen.dart';
import 'screens/courtroom_screen.dart';

void main() {
  runApp(const AoApp());
}

class AoApp extends StatelessWidget {
  const AoApp({super.key});

  @override
  Widget build(BuildContext context) {
    return ChangeNotifierProvider(
      create: (_) => EngineState()..start(null),
      child: MaterialApp(
        title: 'Attorney Online',
        debugShowCheckedModeBanner: false,
        theme: ThemeData(
          brightness: Brightness.dark,
          colorScheme: ColorScheme.fromSeed(
            seedColor: const Color(0xFF1A5276),
            brightness: Brightness.dark,
          ),
          useMaterial3: true,
        ),
        home: const ScreenRouter(),
      ),
    );
  }
}

/// Routes to the correct screen widget based on the engine's active screen.
class ScreenRouter extends StatelessWidget {
  const ScreenRouter({super.key});

  @override
  Widget build(BuildContext context) {
    final engine = context.watch<EngineState>();

    return switch (engine.activeScreen) {
      'server_list' => const ServerListScreen(),
      'char_select' => const CharSelectScreen(),
      'courtroom' => const CourtroomScreen(),
      _ => const Scaffold(
          body: Center(child: CircularProgressIndicator()),
        ),
    };
  }
}

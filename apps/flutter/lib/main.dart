import 'package:flutter/cupertino.dart';
import 'package:provider/provider.dart';

import 'engine_state.dart';
import 'screens/server_list_screen.dart';
import 'screens/char_select_screen.dart';
import 'screens/courtroom_screen.dart';
import 'widgets/platform/platform_widgets.dart';

void main() {
  runApp(const AoApp());
}

class AoApp extends StatelessWidget {
  const AoApp({super.key});

  @override
  Widget build(BuildContext context) {
    return ChangeNotifierProvider(
      create: (_) => EngineState()..start(null),
      // CupertinoApp stays — it's a platform-level choice, not a widget.
      child: CupertinoApp(
        title: 'Attorney Online',
        debugShowCheckedModeBanner: false,
        theme: CupertinoThemeData(
          brightness: Brightness.dark,
          primaryColor: PlatformColors.primary,
          scaffoldBackgroundColor: PlatformColors.background,
          barBackgroundColor: const Color(0xF01C1C1E),
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
      _ => const PlatformPageScaffold(
          child: Center(child: PlatformActivityIndicator()),
        ),
    };
  }
}

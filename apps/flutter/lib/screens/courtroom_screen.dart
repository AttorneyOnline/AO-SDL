import 'package:flutter/material.dart';
import 'package:provider/provider.dart';

import '../bridge/native_bridge.dart';
import '../engine_state.dart';
import '../widgets/courtroom_viewport.dart';
import '../widgets/emote_selector.dart';
import '../widgets/ic_chat.dart';
import '../widgets/ic_log.dart';
import '../widgets/interjection_bar.dart';
import '../widgets/music_list.dart';
import '../widgets/ooc_chat.dart';
import '../widgets/side_select.dart';

/// Main courtroom screen — mirrors apps/sdl/ui/controllers/CourtroomController.
///
/// Layout (portrait mobile):
///   ┌──────────────────┐
///   │  Courtroom View  │  ← native texture from IRenderer
///   ├──────────────────┤
///   │  Interjections   │
///   ├──────────────────┤
///   │  IC Chat Input   │
///   ├──────────────────┤
///   │  Tabbed Panel    │  ← Emotes / IC Log / OOC / Music
///   └──────────────────┘
class CourtroomScreen extends StatelessWidget {
  const CourtroomScreen({super.key});

  @override
  Widget build(BuildContext context) {
    context.watch<EngineState>();
    if (AoBridge.courtroomLoading()) {
      return const Scaffold(
        body: Center(child: Text('Loading character data...')),
      );
    }

    return Scaffold(
      appBar: AppBar(
        title: Text(AoBridge.courtroomCharacter()),
        leading: IconButton(
          icon: const Icon(Icons.swap_horiz),
          tooltip: 'Change Character',
          onPressed: () => AoBridge.navPop(),
        ),
        actions: [
          IconButton(
            icon: const Icon(Icons.logout),
            tooltip: 'Disconnect',
            onPressed: () => AoBridge.navPopToRoot(),
          ),
        ],
      ),
      body: const _CourtroomBody(),
    );
  }
}

class _CourtroomBody extends StatelessWidget {
  const _CourtroomBody();

  @override
  Widget build(BuildContext context) {
    return Column(
      children: [
        // Courtroom viewport — 4:3 aspect ratio
        const AspectRatio(
          aspectRatio: 4 / 3,
          child: CourtroomViewport(),
        ),

        // Interjection buttons
        const InterjectionBar(),

        // Side select + IC input
        const Padding(
          padding: EdgeInsets.symmetric(horizontal: 8, vertical: 4),
          child: SideSelect(),
        ),
        const Padding(
          padding: EdgeInsets.symmetric(horizontal: 8),
          child: IcChat(),
        ),

        const SizedBox(height: 4),

        // Tabbed bottom panel
        const Expanded(child: _BottomTabs()),
      ],
    );
  }
}

class _BottomTabs extends StatelessWidget {
  const _BottomTabs();

  @override
  Widget build(BuildContext context) {
    return DefaultTabController(
      length: 4,
      child: Column(
        children: [
          const TabBar(
            isScrollable: true,
            tabs: [
              Tab(text: 'Emotes'),
              Tab(text: 'IC Log'),
              Tab(text: 'OOC'),
              Tab(text: 'Music'),
            ],
          ),
          Expanded(
            child: TabBarView(
              children: [
                const EmoteSelector(),
                const IcLog(),
                const OocChat(),
                const MusicList(),
              ],
            ),
          ),
        ],
      ),
    );
  }
}

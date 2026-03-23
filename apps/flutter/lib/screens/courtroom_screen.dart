import 'package:flutter/widgets.dart';
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
import '../widgets/platform/platform_widgets.dart';
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
      return PlatformPageScaffold(
        child: Center(
            child: Text('Loading character data...',
                style: TextStyle(color: PlatformColors.text))),
      );
    }

    return PlatformPageScaffold(
      title: AoBridge.courtroomCharacter(),
      leading: PlatformNavButton(
        icon: PlatformIcons.swap,
        onPressed: () => AoBridge.navPop(),
      ),
      trailing: PlatformNavButton(
        icon: PlatformIcons.logout,
        onPressed: () => AoBridge.navPopToRoot(),
      ),
      child: const SafeArea(child: _CourtroomBody()),
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

class _BottomTabs extends StatefulWidget {
  const _BottomTabs();

  @override
  State<_BottomTabs> createState() => _BottomTabsState();
}

class _BottomTabsState extends State<_BottomTabs> {
  int _selectedTab = 0;

  static const _tabLabels = <int, Widget>{
    0: Text('Emotes'),
    1: Text('IC Log'),
    2: Text('OOC'),
    3: Text('Music'),
  };

  @override
  Widget build(BuildContext context) {
    return Column(
      children: [
        Padding(
          padding: const EdgeInsets.symmetric(horizontal: 16, vertical: 4),
          child: SizedBox(
            width: double.infinity,
            child: PlatformSegmentedControl<int>(
              groupValue: _selectedTab,
              children: _tabLabels,
              onValueChanged: (value) {
                if (value != null) setState(() => _selectedTab = value);
              },
            ),
          ),
        ),
        Expanded(
          child: IndexedStack(
            index: _selectedTab,
            children: const [
              EmoteSelector(),
              IcLog(),
              OocChat(),
              MusicList(),
            ],
          ),
        ),
      ],
    );
  }
}

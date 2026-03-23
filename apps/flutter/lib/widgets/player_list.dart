import 'package:flutter/widgets.dart';

import '../bridge/native_bridge.dart';
import 'platform/platform_widgets.dart';

/// Online player list — mirrors apps/sdl/ui/widgets/PlayerListWidget.
class PlayerList extends StatelessWidget {
  const PlayerList({super.key});

  @override
  Widget build(BuildContext context) {
    final count = AoBridge.playerCount();

    if (count == 0) {
      return Center(
          child: Text('No players',
              style: TextStyle(color: PlatformColors.text)));
    }

    return Column(
      crossAxisAlignment: CrossAxisAlignment.start,
      children: [
        Padding(
          padding: const EdgeInsets.all(8),
          child: Text(
            '$count player${count == 1 ? '' : 's'} online',
            style: TextStyle(
                color: PlatformColors.secondaryText, fontSize: 13),
          ),
        ),
        Expanded(
          child: ListView.builder(
            itemCount: count,
            padding: const EdgeInsets.symmetric(horizontal: 8),
            itemBuilder: (context, index) {
              final charname = AoBridge.playerCharname(index);
              final character = AoBridge.playerCharacter(index);
              final name = AoBridge.playerName(index);
              final id = AoBridge.playerId(index);

              String display;
              if (charname.isNotEmpty) {
                display = charname;
              } else if (character.isNotEmpty) {
                display = character;
              } else {
                display = '';
              }

              if (name.isNotEmpty) {
                if (display.isEmpty) {
                  display = name;
                } else {
                  display = '$display ($name)';
                }
              }

              if (display.isEmpty) {
                display = 'Player $id';
              }

              return Padding(
                padding: const EdgeInsets.symmetric(vertical: 2),
                child: Row(
                  children: [
                    Icon(PlatformIcons.person,
                        size: 16, color: PlatformColors.secondaryText),
                    const SizedBox(width: 8),
                    Expanded(
                      child: Text(display,
                          maxLines: 1,
                          overflow: TextOverflow.ellipsis,
                          style: TextStyle(
                              color: PlatformColors.text)),
                    ),
                  ],
                ),
              );
            },
          ),
        ),
      ],
    );
  }
}

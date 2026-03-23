import 'package:flutter/material.dart';

import '../bridge/native_bridge.dart';

/// Music track list — mirrors apps/sdl/ui/widgets/MusicAreaWidget.
class MusicList extends StatelessWidget {
  const MusicList({super.key});

  @override
  Widget build(BuildContext context) {
    final count = AoBridge.musicCount();

    if (count == 0) {
      return const Center(child: Text('No music tracks'));
    }

    return ListView.builder(
      itemCount: count,
      itemBuilder: (context, index) {
        final name = AoBridge.musicName(index);
        return ListTile(
          dense: true,
          leading: const Icon(Icons.music_note, size: 18),
          title: Text(name, maxLines: 1, overflow: TextOverflow.ellipsis),
          onTap: () => AoBridge.musicPlay(index),
        );
      },
    );
  }
}

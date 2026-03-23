import 'dart:ffi';
import 'dart:typed_data';

import 'package:flutter/material.dart';
import 'package:provider/provider.dart';

import '../bridge/native_bridge.dart';
import '../engine_state.dart';
import '../screens/char_select_screen.dart' show rgbaToRgbaBmp;

/// Emote grid — mirrors apps/sdl/ui/widgets/EmoteSelectorWidget.
class EmoteSelector extends StatefulWidget {
  const EmoteSelector({super.key});

  @override
  State<EmoteSelector> createState() => _EmoteSelectorState();
}

class _EmoteSelectorState extends State<EmoteSelector> {
  int _selected = 0;
  int _cachedCount = 0;
  final List<String> _comments = [];
  final List<Uint8List?> _iconBmps = [];

  void _refreshEmotes() {
    final count = AoBridge.courtroomEmoteCount();
    if (count != _cachedCount) {
      _cachedCount = count;
      _comments.clear();
      _iconBmps.clear();
      for (var i = 0; i < count; i++) {
        _comments.add(AoBridge.courtroomEmoteComment(i));
        _iconBmps.add(null);
      }
    }

    // Check for newly loaded icons
    for (var i = 0; i < _cachedCount; i++) {
      if (_iconBmps[i] != null) continue;
      if (!AoBridge.courtroomEmoteIconReady(i)) continue;

      final w = AoBridge.courtroomEmoteIconWidth(i);
      final h = AoBridge.courtroomEmoteIconHeight(i);
      final ptr = AoBridge.courtroomEmoteIconPixels(i);
      if (ptr != nullptr && w > 0 && h > 0) {
        final rgba = ptr.asTypedList(w * h * 4);
        _iconBmps[i] = rgbaToRgbaBmp(rgba, w, h);
      }
    }
  }

  @override
  Widget build(BuildContext context) {
    context.watch<EngineState>();
    _refreshEmotes();

    if (_cachedCount == 0) {
      return const Center(child: Text('No emotes'));
    }

    return GridView.builder(
      padding: const EdgeInsets.all(4),
      gridDelegate: const SliverGridDelegateWithMaxCrossAxisExtent(
        maxCrossAxisExtent: 80,
        mainAxisSpacing: 4,
        crossAxisSpacing: 4,
        childAspectRatio: 1,
      ),
      itemCount: _cachedCount,
      itemBuilder: (context, index) {
        final isSelected = index == _selected;
        final bmp = _iconBmps[index];

        Widget content;
        if (bmp != null) {
          content = Tooltip(
            message: _comments[index],
            child: Image.memory(bmp, fit: BoxFit.contain, gaplessPlayback: true),
          );
        } else {
          content = Center(
            child: Text(
              _comments[index],
              textAlign: TextAlign.center,
              maxLines: 2,
              overflow: TextOverflow.ellipsis,
              style: const TextStyle(fontSize: 10),
            ),
          );
        }

        return GestureDetector(
          onTap: () {
            setState(() => _selected = index);
            AoBridge.icSetEmote(index);
          },
          child: Container(
            decoration: BoxDecoration(
              color: isSelected
                  ? Theme.of(context).colorScheme.primaryContainer
                  : Theme.of(context).colorScheme.surfaceContainerHighest,
              border: Border.all(
                color: isSelected
                    ? Theme.of(context).colorScheme.primary
                    : Colors.transparent,
                width: 2,
              ),
              borderRadius: BorderRadius.circular(4),
            ),
            clipBehavior: Clip.antiAlias,
            child: content,
          ),
        );
      },
    );
  }
}

import 'dart:ffi';
import 'dart:typed_data';

import 'package:flutter/widgets.dart';
import 'package:provider/provider.dart';

import '../bridge/native_bridge.dart';
import '../engine_state.dart';
import '../screens/char_select_screen.dart' show rgbaToRgbaBmp;
import 'platform/platform_widgets.dart';

/// Emote grid — mirrors apps/sdl/ui/widgets/EmoteSelectorWidget.
class EmoteSelector extends StatefulWidget {
  const EmoteSelector({super.key});

  @override
  State<EmoteSelector> createState() => _EmoteSelectorState();
}

class _EmoteSelectorState extends State<EmoteSelector> {
  int _selected = 0;
  int _cachedCount = 0;
  String _cachedCharacter = '';
  final List<String> _comments = [];
  final List<Uint8List?> _iconBmps = [];

  void _refreshEmotes() {
    final count = AoBridge.courtroomEmoteCount();
    final character = AoBridge.courtroomCharacter();

    // Clear cache when character changes OR emote count changes
    if (character != _cachedCharacter || count != _cachedCount) {
      _cachedCount = count;
      _cachedCharacter = character;
      _selected = 0;
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
      return Center(
          child: Text('No emotes',
              style: TextStyle(color: PlatformColors.text)));
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
          content = Image.memory(bmp,
              fit: BoxFit.contain, gaplessPlayback: true);
        } else {
          content = Center(
            child: Text(
              _comments[index],
              textAlign: TextAlign.center,
              maxLines: 2,
              overflow: TextOverflow.ellipsis,
              style: TextStyle(
                  fontSize: 10, color: PlatformColors.text),
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
                  ? PlatformColors.selectedSurface
                  : PlatformColors.separator,
              border: Border.all(
                color: isSelected
                    ? PlatformColors.primary
                    : const Color(0x00000000),
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

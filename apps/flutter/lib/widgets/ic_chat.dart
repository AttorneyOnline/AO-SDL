import 'package:flutter/widgets.dart';
import 'package:provider/provider.dart';

import '../bridge/native_bridge.dart';
import '../engine_state.dart';
import 'platform/platform_widgets.dart';

/// IC chat input — mirrors apps/sdl/ui/widgets/ICChatWidget.
class IcChat extends StatefulWidget {
  const IcChat({super.key});

  @override
  State<IcChat> createState() => _IcChatState();
}

class _IcChatState extends State<IcChat> {
  final _controller = TextEditingController();
  final _shownameController = TextEditingController();
  bool _preAnim = false;
  bool _flip = false;
  int _color = 0;
  String _lastCharacter = '';

  static const _colorNames = <int, String>{
    0: 'White',
    1: 'Green',
    2: 'Red',
    3: 'Orange',
    4: 'Blue',
    5: 'Yellow',
    6: 'Rainbow',
  };

  @override
  void dispose() {
    _controller.dispose();
    _shownameController.dispose();
    super.dispose();
  }

  /// Sync showname from the native bridge when the character changes
  /// or when the showname becomes available after loading.
  void _syncFromBridge() {
    final character = AoBridge.courtroomCharacter();
    final showname = AoBridge.icGetShowname();
    if (character != _lastCharacter ||
        (showname.isNotEmpty && _shownameController.text.isEmpty)) {
      _lastCharacter = character;
      if (showname.isNotEmpty) {
        _shownameController.text = showname;
      }
    }
  }

  void _send() {
    final msg = _controller.text.trim();
    if (msg.isEmpty) return;
    AoBridge.icSend(msg);
    _controller.clear();
  }

  void _showColorPicker(BuildContext context) {
    showPlatformActionSheet(
      context: context,
      title: 'Text Color',
      actions: [
        for (final entry in _colorNames.entries)
          PlatformSheetAction(
            label: entry.value,
            isDefault: entry.key == _color,
            onPressed: () {
              setState(() => _color = entry.key);
              AoBridge.icSetColor(entry.key);
            },
          ),
      ],
      cancelAction: PlatformSheetAction(
        label: 'Cancel',
        onPressed: () {},
      ),
    );
  }

  @override
  Widget build(BuildContext context) {
    context.watch<EngineState>();
    _syncFromBridge();

    return Column(
      mainAxisSize: MainAxisSize.min,
      children: [
        // Showname field + toggles
        Row(
          children: [
            SizedBox(
              width: 120,
              child: PlatformTextField(
                controller: _shownameController,
                placeholder: 'Showname',
                padding:
                    const EdgeInsets.symmetric(horizontal: 8, vertical: 6),
                decoration: BoxDecoration(
                  color: PlatformColors.surface,
                  borderRadius: BorderRadius.circular(6),
                ),
                style: TextStyle(
                    color: PlatformColors.text, fontSize: 14),
                onChanged: (v) => AoBridge.icSetShowname(v),
              ),
            ),
            const SizedBox(width: 8),
            // Pre-anim toggle
            Text('Pre', style: TextStyle(color: PlatformColors.text, fontSize: 13)),
            const SizedBox(width: 4),
            SizedBox(
              height: 28,
              child: FittedBox(
                child: PlatformSwitch(
                  value: _preAnim,
                  onChanged: (v) {
                    setState(() => _preAnim = v);
                    AoBridge.icSetPre(v);
                  },
                ),
              ),
            ),
            const SizedBox(width: 8),
            // Flip toggle
            Text('Flip', style: TextStyle(color: PlatformColors.text, fontSize: 13)),
            const SizedBox(width: 4),
            SizedBox(
              height: 28,
              child: FittedBox(
                child: PlatformSwitch(
                  value: _flip,
                  onChanged: (v) {
                    setState(() => _flip = v);
                    AoBridge.icSetFlip(v);
                  },
                ),
              ),
            ),
            const SizedBox(width: 8),
            // Color selector
            PlatformButton(
              padding:
                  const EdgeInsets.symmetric(horizontal: 8, vertical: 4),
              minimumSize: Size.zero,
              onPressed: () => _showColorPicker(context),
              child: Text(
                _colorNames[_color] ?? 'White',
                style: const TextStyle(fontSize: 13),
              ),
            ),
          ],
        ),
        const SizedBox(height: 4),
        // Message input
        Row(
          children: [
            Expanded(
              child: PlatformTextField(
                controller: _controller,
                placeholder: 'Type your message...',
                padding:
                    const EdgeInsets.symmetric(horizontal: 10, vertical: 8),
                onSubmitted: (_) => _send(),
              ),
            ),
            const SizedBox(width: 8),
            PlatformIconButton(
              icon: PlatformIcons.send,
              onPressed: _send,
              color: PlatformColors.text,
              size: 20,
              padding: const EdgeInsets.all(8),
              fillColor: PlatformColors.primary,
              borderRadius: BorderRadius.circular(20),
            ),
          ],
        ),
      ],
    );
  }
}

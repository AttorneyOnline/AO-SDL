import 'package:flutter/widgets.dart';
import 'package:provider/provider.dart';

import '../bridge/native_bridge.dart';
import '../engine_state.dart';
import 'platform/platform_widgets.dart';

/// Court position selector — mirrors apps/sdl/ui/widgets/SideSelectWidget.
class SideSelect extends StatefulWidget {
  const SideSelect({super.key});

  @override
  State<SideSelect> createState() => _SideSelectState();
}

class _SideSelectState extends State<SideSelect> {
  int _selected = 2; // default: wit
  String _lastCharacter = '';

  static const _sides = ['Def', 'Pro', 'Wit', 'Jud', 'Jur', 'Sea', 'Hlp'];

  /// Sync side selection from native bridge when the character changes.
  void _syncFromBridge() {
    final character = AoBridge.courtroomCharacter();
    if (character != _lastCharacter) {
      _lastCharacter = character;
      _selected = AoBridge.icGetSide();
    }
  }

  @override
  Widget build(BuildContext context) {
    context.watch<EngineState>();
    _syncFromBridge();

    return SizedBox(
      width: double.infinity,
      child: PlatformSegmentedControl<int>(
        groupValue: _selected,
        children: {
          for (var i = 0; i < _sides.length; i++)
            i: Padding(
              padding: const EdgeInsets.symmetric(horizontal: 2),
              child: Text(_sides[i], style: const TextStyle(fontSize: 12)),
            ),
        },
        onValueChanged: (value) {
          if (value == null) return;
          setState(() => _selected = value);
          AoBridge.icSetSide(value);
        },
      ),
    );
  }
}

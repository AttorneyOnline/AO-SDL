import 'package:flutter/material.dart';

import '../bridge/native_bridge.dart';

/// Court position selector — mirrors apps/sdl/ui/widgets/SideSelectWidget.
class SideSelect extends StatefulWidget {
  const SideSelect({super.key});

  @override
  State<SideSelect> createState() => _SideSelectState();
}

class _SideSelectState extends State<SideSelect> {
  int _selected = 2; // default: wit

  static const _sides = ['Def', 'Pro', 'Wit', 'Jud', 'Jur', 'Sea', 'Hlp'];

  @override
  Widget build(BuildContext context) {
    return SegmentedButton<int>(
      segments: [
        for (var i = 0; i < _sides.length; i++)
          ButtonSegment(value: i, label: Text(_sides[i])),
      ],
      selected: {_selected},
      onSelectionChanged: (selection) {
        final val = selection.first;
        setState(() => _selected = val);
        AoBridge.icSetSide(val);
      },
      showSelectedIcon: false,
      style: const ButtonStyle(
        visualDensity: VisualDensity.compact,
      ),
    );
  }
}

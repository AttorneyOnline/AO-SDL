import 'package:flutter/material.dart';

import '../bridge/native_bridge.dart';

/// Interjection buttons — mirrors apps/sdl/ui/widgets/InterjectionWidget.
class InterjectionBar extends StatefulWidget {
  const InterjectionBar({super.key});

  @override
  State<InterjectionBar> createState() => _InterjectionBarState();
}

class _InterjectionBarState extends State<InterjectionBar> {
  int _active = 0; // 0=none, 1=holdit, 2=objection, 3=takethat

  void _toggle(int type) {
    setState(() => _active = _active == type ? 0 : type);
    AoBridge.icSetInterjection(_active);
  }

  @override
  Widget build(BuildContext context) {
    return Padding(
      padding: const EdgeInsets.symmetric(horizontal: 8, vertical: 4),
      child: Row(
        mainAxisAlignment: MainAxisAlignment.spaceEvenly,
        children: [
          _InterjectionButton(
            label: 'Hold It!',
            color: Colors.orange,
            active: _active == 1,
            onTap: () => _toggle(1),
          ),
          _InterjectionButton(
            label: 'Objection!',
            color: Colors.red,
            active: _active == 2,
            onTap: () => _toggle(2),
          ),
          _InterjectionButton(
            label: 'Take That!',
            color: Colors.blue,
            active: _active == 3,
            onTap: () => _toggle(3),
          ),
        ],
      ),
    );
  }
}

class _InterjectionButton extends StatelessWidget {
  final String label;
  final Color color;
  final bool active;
  final VoidCallback onTap;

  const _InterjectionButton({
    required this.label,
    required this.color,
    required this.active,
    required this.onTap,
  });

  @override
  Widget build(BuildContext context) {
    return Expanded(
      child: Padding(
        padding: const EdgeInsets.symmetric(horizontal: 2),
        child: active
            ? FilledButton(
                style: FilledButton.styleFrom(backgroundColor: color),
                onPressed: onTap,
                child: Text(label, style: const TextStyle(color: Colors.white)),
              )
            : OutlinedButton(
                style: OutlinedButton.styleFrom(foregroundColor: color),
                onPressed: onTap,
                child: Text(label),
              ),
      ),
    );
  }
}

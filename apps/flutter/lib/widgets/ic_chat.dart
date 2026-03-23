import 'package:flutter/material.dart';

import '../bridge/native_bridge.dart';

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

  @override
  void dispose() {
    _controller.dispose();
    _shownameController.dispose();
    super.dispose();
  }

  void _send() {
    final msg = _controller.text.trim();
    if (msg.isEmpty) return;
    AoBridge.icSend(msg);
    _controller.clear();
  }

  @override
  Widget build(BuildContext context) {
    return Column(
      mainAxisSize: MainAxisSize.min,
      children: [
        // Showname field
        Row(
          children: [
            SizedBox(
              width: 120,
              child: TextField(
                controller: _shownameController,
                decoration: const InputDecoration(
                  hintText: 'Showname',
                  isDense: true,
                  border: OutlineInputBorder(),
                  contentPadding:
                      EdgeInsets.symmetric(horizontal: 8, vertical: 6),
                ),
                onChanged: (v) => AoBridge.icSetShowname(v),
              ),
            ),
            const SizedBox(width: 8),
            // Pre-anim toggle
            FilterChip(
              label: const Text('Pre'),
              selected: _preAnim,
              onSelected: (v) {
                setState(() => _preAnim = v);
                AoBridge.icSetPre(v);
              },
            ),
            const SizedBox(width: 4),
            // Flip toggle
            FilterChip(
              label: const Text('Flip'),
              selected: _flip,
              onSelected: (v) {
                setState(() => _flip = v);
                AoBridge.icSetFlip(v);
              },
            ),
            const SizedBox(width: 4),
            // Color selector
            DropdownButton<int>(
              value: _color,
              isDense: true,
              items: const [
                DropdownMenuItem(value: 0, child: Text('White')),
                DropdownMenuItem(value: 1, child: Text('Green')),
                DropdownMenuItem(value: 2, child: Text('Red')),
                DropdownMenuItem(value: 3, child: Text('Orange')),
                DropdownMenuItem(value: 4, child: Text('Blue')),
                DropdownMenuItem(value: 5, child: Text('Yellow')),
                DropdownMenuItem(value: 6, child: Text('Rainbow')),
              ],
              onChanged: (v) {
                if (v == null) return;
                setState(() => _color = v);
                AoBridge.icSetColor(v);
              },
            ),
          ],
        ),
        const SizedBox(height: 4),
        // Message input
        Row(
          children: [
            Expanded(
              child: TextField(
                controller: _controller,
                decoration: const InputDecoration(
                  hintText: 'Type your message...',
                  isDense: true,
                  border: OutlineInputBorder(),
                ),
                onSubmitted: (_) => _send(),
              ),
            ),
            const SizedBox(width: 8),
            IconButton.filled(
              onPressed: _send,
              icon: const Icon(Icons.send),
            ),
          ],
        ),
      ],
    );
  }
}

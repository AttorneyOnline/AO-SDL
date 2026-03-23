import 'package:flutter/widgets.dart';

import '../bridge/native_bridge.dart';
import 'platform/platform_widgets.dart';

/// OOC (out-of-character) chat — mirrors apps/sdl/ui/widgets/ChatWidget.
class OocChat extends StatefulWidget {
  const OocChat({super.key});

  @override
  State<OocChat> createState() => _OocChatState();
}

class _OocChatState extends State<OocChat> {
  final List<({String name, String text})> _messages = [];
  final _nameController = TextEditingController();
  final _messageController = TextEditingController();
  final _scrollController = ScrollController();

  @override
  void dispose() {
    _nameController.dispose();
    _messageController.dispose();
    _scrollController.dispose();
    super.dispose();
  }

  void _pollMessages() {
    final msgs = AoBridge.oocMessages();
    if (msgs.isNotEmpty) {
      setState(() => _messages.addAll(msgs));
      WidgetsBinding.instance.addPostFrameCallback((_) {
        if (_scrollController.hasClients) {
          _scrollController.jumpTo(_scrollController.position.maxScrollExtent);
        }
      });
    }
  }

  void _send() {
    final name = _nameController.text.trim();
    final msg = _messageController.text.trim();
    if (msg.isEmpty) return;
    AoBridge.oocSend(name.isEmpty ? 'Player' : name, msg);
    _messageController.clear();
  }

  @override
  Widget build(BuildContext context) {
    _pollMessages();

    return Column(
      children: [
        Expanded(
          child: _messages.isEmpty
              ? Center(
                  child: Text('No messages',
                      style: TextStyle(color: PlatformColors.text)))
              : ListView.builder(
                  controller: _scrollController,
                  itemCount: _messages.length,
                  padding: const EdgeInsets.all(8),
                  itemBuilder: (context, index) {
                    final msg = _messages[index];
                    return Padding(
                      padding: const EdgeInsets.symmetric(vertical: 2),
                      child: RichText(
                        text: TextSpan(
                          children: [
                            TextSpan(
                              text: '[${msg.name}]: ',
                              style: TextStyle(
                                fontWeight: FontWeight.bold,
                                color: PlatformColors.teal,
                              ),
                            ),
                            TextSpan(
                              text: msg.text,
                              style: TextStyle(
                                color: PlatformColors.text,
                              ),
                            ),
                          ],
                        ),
                      ),
                    );
                  },
                ),
        ),
        Padding(
          padding: const EdgeInsets.all(8),
          child: Row(
            children: [
              SizedBox(
                width: 80,
                child: PlatformTextField(
                  controller: _nameController,
                  placeholder: 'Name',
                  padding: const EdgeInsets.symmetric(
                      horizontal: 8, vertical: 6),
                  decoration: BoxDecoration(
                    color: PlatformColors.surface,
                    borderRadius: BorderRadius.circular(6),
                  ),
                  style: TextStyle(
                      color: PlatformColors.text, fontSize: 14),
                ),
              ),
              const SizedBox(width: 8),
              Expanded(
                child: PlatformTextField(
                  controller: _messageController,
                  placeholder: 'OOC message...',
                  padding: const EdgeInsets.symmetric(
                      horizontal: 10, vertical: 8),
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
        ),
      ],
    );
  }
}

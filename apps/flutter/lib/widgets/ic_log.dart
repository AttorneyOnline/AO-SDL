import 'package:flutter/material.dart';

import '../bridge/native_bridge.dart';

/// IC message log — mirrors apps/sdl/ui/widgets/ICLogWidget.
class IcLog extends StatefulWidget {
  const IcLog({super.key});

  @override
  State<IcLog> createState() => _IcLogState();
}

class _IcLogState extends State<IcLog> {
  final List<({String showname, String message})> _entries = [];
  final _scrollController = ScrollController();

  @override
  void dispose() {
    _scrollController.dispose();
    super.dispose();
  }

  void _pollEntries() {
    final newEntries = AoBridge.icLog();
    if (newEntries.isNotEmpty) {
      setState(() => _entries.addAll(newEntries));
      // Auto-scroll to bottom
      WidgetsBinding.instance.addPostFrameCallback((_) {
        if (_scrollController.hasClients) {
          _scrollController.jumpTo(_scrollController.position.maxScrollExtent);
        }
      });
    }
  }

  @override
  Widget build(BuildContext context) {
    _pollEntries();

    if (_entries.isEmpty) {
      return const Center(child: Text('No messages yet'));
    }

    return ListView.builder(
      controller: _scrollController,
      itemCount: _entries.length,
      padding: const EdgeInsets.all(8),
      itemBuilder: (context, index) {
        final entry = _entries[index];
        return Padding(
          padding: const EdgeInsets.symmetric(vertical: 2),
          child: RichText(
            text: TextSpan(
              children: [
                TextSpan(
                  text: '${entry.showname}: ',
                  style: TextStyle(
                    fontWeight: FontWeight.bold,
                    color: Theme.of(context).colorScheme.primary,
                  ),
                ),
                TextSpan(
                  text: entry.message,
                  style: TextStyle(
                    color: Theme.of(context).colorScheme.onSurface,
                  ),
                ),
              ],
            ),
          ),
        );
      },
    );
  }
}

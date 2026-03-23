import 'package:flutter/widgets.dart';
import 'package:provider/provider.dart';

import '../bridge/native_bridge.dart';
import '../engine_state.dart';
import 'platform/platform_widgets.dart';

/// IC message log — mirrors apps/sdl/ui/widgets/ICLogWidget.
class IcLog extends StatefulWidget {
  const IcLog({super.key});

  @override
  State<IcLog> createState() => _IcLogState();
}

class _IcLogState extends State<IcLog> {
  final List<({String showname, String message})> _entries = [];
  final _scrollController = ScrollController();
  EngineState? _engine;

  @override
  void didChangeDependencies() {
    super.didChangeDependencies();
    // Save reference in didChangeDependencies so it's safe to use in dispose.
    final engine = context.read<EngineState>();
    if (_engine != engine) {
      _engine?.removeListener(_onEngineStateChanged);
      _engine = engine;
      _engine!.addListener(_onEngineStateChanged);
    }
  }

  @override
  void dispose() {
    _engine?.removeListener(_onEngineStateChanged);
    _scrollController.dispose();
    super.dispose();
  }

  void _onEngineStateChanged() {
    final newEntries = AoBridge.icLog();
    if (newEntries.isNotEmpty) {
      setState(() => _entries.addAll(newEntries));
      WidgetsBinding.instance.addPostFrameCallback((_) {
        if (_scrollController.hasClients) {
          _scrollController.jumpTo(_scrollController.position.maxScrollExtent);
        }
      });
    }
  }

  @override
  Widget build(BuildContext context) {
    // Watch EngineState so the widget is part of the dependency tree,
    // but do NOT poll entries here — that happens in _onEngineStateChanged.
    context.watch<EngineState>();

    if (_entries.isEmpty) {
      return Center(
          child: Text('No messages yet',
              style: TextStyle(color: PlatformColors.text)));
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
                    color: PlatformColors.primary,
                  ),
                ),
                TextSpan(
                  text: entry.message,
                  style: TextStyle(
                    color: PlatformColors.text,
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

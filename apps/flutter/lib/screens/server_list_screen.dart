import 'package:flutter/material.dart';
import 'package:provider/provider.dart';

import '../bridge/native_bridge.dart';
import '../engine_state.dart';

/// Server browser — mirrors apps/sdl/ui/widgets/ServerListWidget.
class ServerListScreen extends StatefulWidget {
  const ServerListScreen({super.key});

  @override
  State<ServerListScreen> createState() => _ServerListScreenState();
}

class _ServerListScreenState extends State<ServerListScreen> {
  final _directConnectController = TextEditingController();
  int _selectedIndex = -1;

  @override
  void dispose() {
    _directConnectController.dispose();
    super.dispose();
  }

  void _onDirectConnect() {
    final addr = _directConnectController.text.trim();
    if (addr.isEmpty) return;

    var host = addr;
    var port = 27016; // AO2 default WS port
    final colon = addr.lastIndexOf(':');
    if (colon != -1) {
      host = addr.substring(0, colon);
      port = int.tryParse(addr.substring(colon + 1)) ?? 27016;
    }
    AoBridge.serverDirectConnect(host, port);
  }

  @override
  Widget build(BuildContext context) {
    // Watch EngineState so we rebuild when the engine ticks with new data
    context.watch<EngineState>();
    final serverCount = AoBridge.serverCount();

    return Scaffold(
      appBar: AppBar(
        title: const Text('Attorney Online'),
        centerTitle: true,
      ),
      body: Column(
        children: [
          // Direct connect bar
          Padding(
            padding: const EdgeInsets.all(8.0),
            child: Row(
              children: [
                Expanded(
                  child: TextField(
                    controller: _directConnectController,
                    decoration: const InputDecoration(
                      hintText: 'host:port',
                      isDense: true,
                      border: OutlineInputBorder(),
                    ),
                    onSubmitted: (_) => _onDirectConnect(),
                  ),
                ),
                const SizedBox(width: 8),
                FilledButton(
                  onPressed: _onDirectConnect,
                  child: const Text('Connect'),
                ),
              ],
            ),
          ),

          // Server list header
          if (serverCount == 0)
            const Expanded(
              child: Center(child: Text('Fetching server list...')),
            )
          else
            Expanded(
              child: _buildServerList(serverCount),
            ),
        ],
      ),
    );
  }

  Widget _buildServerList(int count) {
    return ListView.builder(
      itemCount: count,
      itemBuilder: (context, index) {
        final name = AoBridge.serverName(index);
        final desc = AoBridge.serverDescription(index);
        final players = AoBridge.serverPlayers(index);
        final hasWs = AoBridge.serverHasWs(index);
        final isSelected = index == _selectedIndex;

        return ListTile(
          selected: isSelected,
          enabled: hasWs,
          title: Text(
            name,
            maxLines: 1,
            overflow: TextOverflow.ellipsis,
          ),
          subtitle: Text(
            desc,
            maxLines: 2,
            overflow: TextOverflow.ellipsis,
          ),
          trailing: Text(
            '$players',
            style: Theme.of(context).textTheme.titleMedium,
          ),
          onTap: hasWs
              ? () {
                  setState(() => _selectedIndex = index);
                  AoBridge.serverSelect(index);
                }
              : null,
        );
      },
    );
  }
}

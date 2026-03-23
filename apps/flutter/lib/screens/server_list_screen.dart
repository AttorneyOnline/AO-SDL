import 'package:flutter/widgets.dart';
import 'package:provider/provider.dart';

import '../bridge/native_bridge.dart';
import '../engine_state.dart';
import '../widgets/platform/platform_widgets.dart';

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

    return PlatformPageScaffold(
      title: 'Attorney Online',
      child: SafeArea(
        child: Column(
          children: [
            // Direct connect bar
            Padding(
              padding: const EdgeInsets.all(8.0),
              child: Row(
                children: [
                  Expanded(
                    child: PlatformTextField(
                      controller: _directConnectController,
                      placeholder: 'host:port',
                      padding: const EdgeInsets.symmetric(
                          horizontal: 10, vertical: 8),
                      onSubmitted: (_) => _onDirectConnect(),
                    ),
                  ),
                  const SizedBox(width: 8),
                  PlatformFilledButton(
                    padding: const EdgeInsets.symmetric(
                        horizontal: 16, vertical: 8),
                    onPressed: _onDirectConnect,
                    child: const Text('Connect'),
                  ),
                ],
              ),
            ),

            // Server list header
            if (serverCount == 0)
              Expanded(
                child: Center(
                    child: Text('Fetching server list...',
                        style: TextStyle(color: PlatformColors.text))),
              )
            else
              Expanded(
                child: _buildServerList(serverCount),
              ),
          ],
        ),
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

        return GestureDetector(
          onTap: hasWs
              ? () {
                  setState(() => _selectedIndex = index);
                  AoBridge.serverSelect(index);
                }
              : null,
          child: Container(
            padding:
                const EdgeInsets.symmetric(horizontal: 16, vertical: 10),
            decoration: BoxDecoration(
              color: isSelected ? PlatformColors.selectedSurface : null,
              border: Border(
                bottom: BorderSide(
                    color: PlatformColors.separator, width: 0.5),
              ),
            ),
            child: Opacity(
              opacity: hasWs ? 1.0 : 0.4,
              child: Row(
                children: [
                  Expanded(
                    child: Column(
                      crossAxisAlignment: CrossAxisAlignment.start,
                      children: [
                        Text(
                          name,
                          maxLines: 1,
                          overflow: TextOverflow.ellipsis,
                          style: TextStyle(
                            color: PlatformColors.text,
                            fontSize: 16,
                          ),
                        ),
                        const SizedBox(height: 2),
                        Text(
                          desc,
                          maxLines: 2,
                          overflow: TextOverflow.ellipsis,
                          style: TextStyle(
                            color: PlatformColors.secondaryText,
                            fontSize: 13,
                          ),
                        ),
                      ],
                    ),
                  ),
                  const SizedBox(width: 8),
                  Text(
                    '$players',
                    style: TextStyle(
                      color: PlatformColors.text,
                      fontSize: 17,
                      fontWeight: FontWeight.w500,
                    ),
                  ),
                ],
              ),
            ),
          ),
        );
      },
    );
  }
}

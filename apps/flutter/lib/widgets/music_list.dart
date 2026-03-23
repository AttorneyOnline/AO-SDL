import 'package:flutter/widgets.dart';

import '../bridge/native_bridge.dart';
import 'platform/platform_widgets.dart';

/// Music & Area list — mirrors apps/sdl/ui/widgets/MusicAreaWidget.
class MusicList extends StatefulWidget {
  const MusicList({super.key});

  @override
  State<MusicList> createState() => _MusicListState();
}

class _MusicListState extends State<MusicList> {
  final _searchController = TextEditingController();
  String _searchFilter = '';
  int _selectedTab = 0; // 0 = Music, 1 = Areas

  @override
  void initState() {
    super.initState();
    _searchController.addListener(() {
      setState(() => _searchFilter = _searchController.text.toLowerCase());
    });
  }

  @override
  void dispose() {
    _searchController.dispose();
    super.dispose();
  }

  @override
  Widget build(BuildContext context) {
    final nowPlaying = AoBridge.nowPlaying();

    return Column(
      children: [
        Padding(
          padding: const EdgeInsets.symmetric(horizontal: 16, vertical: 4),
          child: SizedBox(
            width: double.infinity,
            child: PlatformSegmentedControl<int>(
              groupValue: _selectedTab,
              children: const {
                0: Text('Music'),
                1: Text('Areas'),
              },
              onValueChanged: (value) {
                if (value != null) setState(() => _selectedTab = value);
              },
            ),
          ),
        ),
        Expanded(
          child: IndexedStack(
            index: _selectedTab,
            children: [
              _buildMusicTab(nowPlaying),
              _buildAreaTab(),
            ],
          ),
        ),
      ],
    );
  }

  Widget _buildMusicTab(String nowPlaying) {
    final count = AoBridge.musicCount();

    return Column(
      children: [
        Padding(
          padding: const EdgeInsets.all(8),
          child: PlatformTextField(
            controller: _searchController,
            placeholder: 'Search...',
            padding:
                const EdgeInsets.symmetric(horizontal: 10, vertical: 8),
            prefix: Padding(
              padding: const EdgeInsets.only(left: 8),
              child: Icon(PlatformIcons.search,
                  size: 16, color: PlatformColors.secondaryText),
            ),
          ),
        ),
        if (nowPlaying.isNotEmpty)
          Padding(
            padding: const EdgeInsets.symmetric(horizontal: 8),
            child: Row(
              children: [
                Icon(PlatformIcons.play,
                    size: 14, color: PlatformColors.primary),
                const SizedBox(width: 4),
                Expanded(
                  child: Text(
                    'Now: $nowPlaying',
                    style: TextStyle(
                        color: PlatformColors.primary, fontSize: 13),
                    maxLines: 1,
                    overflow: TextOverflow.ellipsis,
                  ),
                ),
              ],
            ),
          ),
        Expanded(
          child: count == 0
              ? Center(
                  child: Text('No music tracks',
                      style: TextStyle(color: PlatformColors.text)))
              : ListView.builder(
                  itemCount: count,
                  itemBuilder: (context, index) {
                    final name = AoBridge.musicName(index);
                    // Filter
                    if (_searchFilter.isNotEmpty &&
                        !name.toLowerCase().contains(_searchFilter)) {
                      return const SizedBox.shrink();
                    }
                    // Category headers have no file extension
                    final isCategory =
                        name.isNotEmpty && !name.contains('.');
                    if (isCategory) {
                      return Padding(
                        padding: const EdgeInsets.only(
                            top: 12, left: 16, right: 16, bottom: 4),
                        child: Text(
                          name,
                          style: TextStyle(
                            fontWeight: FontWeight.bold,
                            color: PlatformColors.primary,
                            fontSize: 14,
                          ),
                        ),
                      );
                    }
                    return GestureDetector(
                      onTap: () => AoBridge.musicPlay(index),
                      child: Container(
                        padding: const EdgeInsets.symmetric(
                            horizontal: 16, vertical: 8),
                        child: Row(
                          children: [
                            Icon(PlatformIcons.musicNote,
                                size: 16,
                                color: PlatformColors.secondaryText),
                            const SizedBox(width: 8),
                            Expanded(
                              child: Text(
                                name,
                                maxLines: 1,
                                overflow: TextOverflow.ellipsis,
                                style: TextStyle(
                                    color: PlatformColors.text,
                                    fontSize: 14),
                              ),
                            ),
                          ],
                        ),
                      ),
                    );
                  },
                ),
        ),
      ],
    );
  }

  Widget _buildAreaTab() {
    final count = AoBridge.areaCount();

    if (count == 0) {
      return Center(
          child: Text('No areas',
              style: TextStyle(color: PlatformColors.text)));
    }

    return ListView.builder(
      itemCount: count,
      itemBuilder: (context, index) {
        final name = AoBridge.areaName(index);
        final status = AoBridge.areaStatus(index);
        final players = AoBridge.areaPlayers(index);
        final cm = AoBridge.areaCm(index);
        final lock = AoBridge.areaLock(index);
        final isLocked = lock == 'LOCKED';

        return GestureDetector(
          onTap: () => AoBridge.musicPlayByName(name),
          child: Container(
            padding:
                const EdgeInsets.symmetric(horizontal: 16, vertical: 8),
            decoration: BoxDecoration(
              border: Border(
                bottom:
                    BorderSide(color: PlatformColors.separator, width: 0.5),
              ),
            ),
            child: Column(
              crossAxisAlignment: CrossAxisAlignment.start,
              children: [
                Text(
                  name,
                  style: TextStyle(
                    color: isLocked
                        ? PlatformColors.secondaryText
                        : PlatformColors.text,
                    fontSize: 15,
                  ),
                ),
                const SizedBox(height: 2),
                Row(
                  children: [
                    Text(
                      status,
                      style: TextStyle(
                        color: _statusColor(status),
                        fontSize: 12,
                      ),
                    ),
                    if (players >= 0) ...[
                      const SizedBox(width: 8),
                      Text('($players)',
                          style: TextStyle(
                              fontSize: 12,
                              color: PlatformColors.secondaryText)),
                    ],
                    if (cm.isNotEmpty &&
                        cm != 'FREE' &&
                        cm != 'Unknown') ...[
                      const SizedBox(width: 8),
                      Text('CM: $cm',
                          style: TextStyle(
                              fontSize: 12,
                              color: PlatformColors.secondaryText)),
                    ],
                    const Spacer(),
                    if (isLocked)
                      Icon(PlatformIcons.lock,
                          size: 14,
                          color: PlatformColors.secondaryText),
                  ],
                ),
              ],
            ),
          ),
        );
      },
    );
  }

  Color _statusColor(String status) {
    switch (status) {
      case 'LOOKING-FOR-PLAYERS':
        return PlatformColors.green;
      case 'CASING':
        return PlatformColors.yellow;
      case 'RECESS':
        return PlatformColors.blue;
      case 'RP':
        return PlatformColors.purple;
      case 'GAMING':
        return PlatformColors.orange;
      default:
        return PlatformColors.secondaryText;
    }
  }
}

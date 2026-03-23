import 'dart:ffi';
import 'dart:typed_data';

import 'package:flutter/widgets.dart';
import 'package:provider/provider.dart';

import '../bridge/native_bridge.dart';
import '../engine_state.dart';
import '../widgets/platform/platform_widgets.dart';

/// Character selection grid — mirrors apps/sdl/ui/widgets/CharSelectWidget.
class CharSelectScreen extends StatelessWidget {
  const CharSelectScreen({super.key});

  @override
  Widget build(BuildContext context) {
    context.watch<EngineState>();
    final charCount = AoBridge.charCount();

    return PlatformPageScaffold(
      title: 'Select Character',
      leading: PlatformNavButton(
        icon: PlatformIcons.back,
        onPressed: () => AoBridge.navPopToRoot(),
      ),
      child: SafeArea(
        child: charCount == 0
            ? Center(
                child: Text('Loading characters...',
                    style: TextStyle(color: PlatformColors.text)))
            : GridView.builder(
                padding: const EdgeInsets.all(8),
                gridDelegate:
                    const SliverGridDelegateWithMaxCrossAxisExtent(
                  maxCrossAxisExtent: 100,
                  mainAxisSpacing: 4,
                  crossAxisSpacing: 4,
                  childAspectRatio: 1,
                ),
                itemCount: charCount,
                itemBuilder: (context, index) {
                  return _CharacterTile(index: index);
                },
              ),
      ),
    );
  }
}

class _CharacterTile extends StatelessWidget {
  final int index;
  const _CharacterTile({required this.index});

  @override
  Widget build(BuildContext context) {
    final folder = AoBridge.charFolder(index);
    final taken = AoBridge.charTaken(index);
    final selected = AoBridge.charSelected() == index;
    final hasIcon = AoBridge.charIconReady(index);

    Widget iconWidget;
    if (hasIcon) {
      final w = AoBridge.charIconWidth(index);
      final h = AoBridge.charIconHeight(index);
      final ptr = AoBridge.charIconPixels(index);
      if (ptr != nullptr && w > 0 && h > 0) {
        // Copy RGBA pixels from native memory into a Dart Uint8List
        final bytes = ptr.asTypedList(w * h * 4);
        iconWidget = Image.memory(
          rgbaToRgbaBmp(bytes, w, h),
          fit: BoxFit.contain,
          gaplessPlayback: true,
        );
      } else {
        iconWidget = _fallbackLabel(folder, taken);
      }
    } else {
      iconWidget = _fallbackLabel(folder, taken);
    }

    return GestureDetector(
      onTap: taken ? null : () => AoBridge.charSelect(index),
      child: Container(
        decoration: BoxDecoration(
          color: selected
              ? PlatformColors.selectedSurface
              : taken
                  ? PlatformColors.separator
                  : PlatformColors.surface,
          border: Border.all(
            color: selected
                ? PlatformColors.primary
                : PlatformColors.outline,
            width: selected ? 2 : 1,
          ),
          borderRadius: BorderRadius.circular(4),
        ),
        clipBehavior: Clip.antiAlias,
        child: iconWidget,
      ),
    );
  }

  static Widget _fallbackLabel(String folder, bool taken) {
    return Center(
      child: Text(
        folder,
        textAlign: TextAlign.center,
        maxLines: 2,
        overflow: TextOverflow.ellipsis,
        style: TextStyle(
          fontSize: 10,
          color: taken
              ? PlatformColors.secondaryText
              : PlatformColors.text,
        ),
      ),
    );
  }
}

/// Encode raw RGBA pixels as a BMP so Image.memory() can decode it.
/// BMP is trivial to construct and avoids pulling in a PNG encoder.
/// Public so other widgets (emote selector, etc.) can reuse it.
Uint8List rgbaToRgbaBmp(Uint8List rgba, int w, int h) {
  // BMP with 32-bit BGRA + alpha channel (BITMAPV4HEADER)
  const headerSize = 14 + 108; // file header + V4 DIB header
  final dataSize = w * h * 4;
  final fileSize = headerSize + dataSize;
  final bmp = Uint8List(fileSize);
  final bd = ByteData.sublistView(bmp);

  // File header
  bmp[0] = 0x42; // 'B'
  bmp[1] = 0x4D; // 'M'
  bd.setUint32(2, fileSize, Endian.little);
  bd.setUint32(10, headerSize, Endian.little);

  // BITMAPV4HEADER (108 bytes)
  bd.setUint32(14, 108, Endian.little); // header size
  bd.setInt32(18, w, Endian.little);
  bd.setInt32(22, h, Endian.little); // positive = bottom-up (BMP default)
  bd.setUint16(26, 1, Endian.little); // planes
  bd.setUint16(28, 32, Endian.little); // bits per pixel
  bd.setUint32(30, 3, Endian.little); // compression = BI_BITFIELDS
  bd.setUint32(34, dataSize, Endian.little);
  bd.setUint32(38, 2835, Endian.little); // h-res
  bd.setUint32(42, 2835, Endian.little); // v-res
  // RGBA channel masks
  bd.setUint32(54, 0x00FF0000, Endian.little); // red
  bd.setUint32(58, 0x0000FF00, Endian.little); // green
  bd.setUint32(62, 0x000000FF, Endian.little); // blue
  bd.setUint32(66, 0xFF000000, Endian.little); // alpha

  // Convert RGBA → BGRA and write pixel data
  for (var i = 0; i < w * h; i++) {
    final si = i * 4;
    final di = headerSize + i * 4;
    bmp[di + 0] = rgba[si + 2]; // B
    bmp[di + 1] = rgba[si + 1]; // G
    bmp[di + 2] = rgba[si + 0]; // R
    bmp[di + 3] = rgba[si + 3]; // A
  }

  return bmp;
}

import 'package:flutter/services.dart';
import 'package:flutter/widgets.dart';

import 'platform/platform_widgets.dart';

/// Courtroom scene viewport — displays the offscreen Metal texture from IRenderer
/// via Flutter's Texture widget.
class CourtroomViewport extends StatefulWidget {
  const CourtroomViewport({super.key});

  @override
  State<CourtroomViewport> createState() => _CourtroomViewportState();
}

class _CourtroomViewportState extends State<CourtroomViewport> {
  static const _channel = MethodChannel('ao_texture');
  int? _textureId;

  @override
  void initState() {
    super.initState();
    _createTexture();
  }

  Future<void> _createTexture() async {
    try {
      final id = await _channel.invokeMethod<int>('create', {
        'width': 1024,
        'height': 768,
      });
      if (mounted) {
        setState(() => _textureId = id);
      }
    } on PlatformException catch (e) {
      debugPrint('Failed to create courtroom texture: $e');
    }
  }

  @override
  void dispose() {
    if (_textureId != null) {
      _channel.invokeMethod('dispose');
    }
    super.dispose();
  }

  @override
  Widget build(BuildContext context) {
    if (_textureId == null) {
      return Container(
        color: const Color(0xFF1A1A33),
        child: const Center(
          child: PlatformActivityIndicator(),
        ),
      );
    }

    return Texture(textureId: _textureId!);
  }
}

/// Platform-agnostic widget abstraction layer.
///
/// All widgets currently resolve to Cupertino implementations.
/// When Android support is added, this file will branch on [isIOS]
/// to return Material equivalents.
library;

import 'dart:io' show Platform;

import 'package:flutter/cupertino.dart';

// ---------------------------------------------------------------------------
// Platform detection
// ---------------------------------------------------------------------------

/// Returns `true` on iOS and macOS (Cupertino targets).
bool get isIOS => Platform.isIOS || Platform.isMacOS;

// ---------------------------------------------------------------------------
// PlatformPageScaffold
// ---------------------------------------------------------------------------

/// A page scaffold with an optional navigation bar.
class PlatformPageScaffold extends StatelessWidget {
  const PlatformPageScaffold({
    super.key,
    this.title,
    this.leading,
    this.trailing,
    required this.child,
  });

  final String? title;
  final Widget? leading;
  final Widget? trailing;
  final Widget child;

  @override
  Widget build(BuildContext context) {
    return CupertinoPageScaffold(
      navigationBar: title != null
          ? CupertinoNavigationBar(
              middle: Text(title!),
              leading: leading,
              trailing: trailing,
            )
          : null,
      child: child,
    );
  }
}

// ---------------------------------------------------------------------------
// PlatformTextField
// ---------------------------------------------------------------------------

/// A styled text field.
class PlatformTextField extends StatelessWidget {
  const PlatformTextField({
    super.key,
    this.controller,
    this.placeholder,
    this.onSubmitted,
    this.onChanged,
    this.style,
    this.placeholderStyle,
    this.padding = const EdgeInsets.symmetric(horizontal: 10, vertical: 8),
    this.decoration,
    this.prefix,
  });

  final TextEditingController? controller;
  final String? placeholder;
  final ValueChanged<String>? onSubmitted;
  final ValueChanged<String>? onChanged;
  final TextStyle? style;
  final TextStyle? placeholderStyle;
  final EdgeInsetsGeometry padding;
  final BoxDecoration? decoration;
  final Widget? prefix;

  @override
  Widget build(BuildContext context) {
    return CupertinoTextField(
      controller: controller,
      placeholder: placeholder,
      onSubmitted: onSubmitted,
      onChanged: onChanged,
      style: style ?? const TextStyle(color: CupertinoColors.white),
      placeholderStyle: placeholderStyle ??
          const TextStyle(color: CupertinoColors.systemGrey),
      padding: padding,
      decoration: decoration ??
          BoxDecoration(
            color: PlatformColors.surface,
            borderRadius: BorderRadius.circular(8),
          ),
      prefix: prefix,
    );
  }
}

// ---------------------------------------------------------------------------
// PlatformButton
// ---------------------------------------------------------------------------

/// A tappable button. When [color] is non-null the button is filled.
class PlatformButton extends StatelessWidget {
  const PlatformButton({
    super.key,
    required this.onPressed,
    required this.child,
    this.color,
    this.padding,
    this.minimumSize,
    this.borderRadius,
  });

  final VoidCallback? onPressed;
  final Widget child;
  final Color? color;
  final EdgeInsetsGeometry? padding;
  final Size? minimumSize;
  final BorderRadius? borderRadius;

  @override
  Widget build(BuildContext context) {
    if (color != null) {
      return CupertinoButton(
        padding: padding,
        color: color,
        borderRadius: borderRadius ?? BorderRadius.circular(8),
        onPressed: onPressed,
        child: child,
      );
    }
    return CupertinoButton(
      padding: padding,
      minimumSize: minimumSize,
      borderRadius: borderRadius,
      onPressed: onPressed,
      child: child,
    );
  }
}

/// A filled button shorthand.
class PlatformFilledButton extends StatelessWidget {
  const PlatformFilledButton({
    super.key,
    required this.onPressed,
    required this.child,
    this.padding,
  });

  final VoidCallback? onPressed;
  final Widget child;
  final EdgeInsetsGeometry? padding;

  @override
  Widget build(BuildContext context) {
    return CupertinoButton.filled(
      padding: padding,
      onPressed: onPressed,
      child: child,
    );
  }
}

// ---------------------------------------------------------------------------
// PlatformIconButton
// ---------------------------------------------------------------------------

/// An icon-only button.
class PlatformIconButton extends StatelessWidget {
  const PlatformIconButton({
    super.key,
    required this.icon,
    required this.onPressed,
    this.color,
    this.size = 22,
    this.padding = EdgeInsets.zero,
    this.fillColor,
    this.borderRadius,
    this.minimumSize,
  });

  final IconData icon;
  final VoidCallback? onPressed;
  final Color? color;
  final double size;
  final EdgeInsetsGeometry padding;
  final Color? fillColor;
  final BorderRadius? borderRadius;
  final Size? minimumSize;

  @override
  Widget build(BuildContext context) {
    return CupertinoButton(
      padding: padding,
      minimumSize: minimumSize ?? Size.zero,
      color: fillColor,
      borderRadius: borderRadius,
      onPressed: onPressed,
      child: Icon(icon, color: color ?? PlatformColors.primary, size: size),
    );
  }
}

// ---------------------------------------------------------------------------
// PlatformSegmentedControl
// ---------------------------------------------------------------------------

/// A sliding segmented control for tab-like selection.
class PlatformSegmentedControl<T extends Object> extends StatelessWidget {
  const PlatformSegmentedControl({
    super.key,
    required this.children,
    required this.groupValue,
    required this.onValueChanged,
  });

  final Map<T, Widget> children;
  final T groupValue;
  final ValueChanged<T?> onValueChanged;

  @override
  Widget build(BuildContext context) {
    return CupertinoSlidingSegmentedControl<T>(
      groupValue: groupValue,
      children: children,
      onValueChanged: onValueChanged,
    );
  }
}

// ---------------------------------------------------------------------------
// PlatformSwitch
// ---------------------------------------------------------------------------

/// A toggle switch.
class PlatformSwitch extends StatelessWidget {
  const PlatformSwitch({
    super.key,
    required this.value,
    required this.onChanged,
    this.activeColor,
  });

  final bool value;
  final ValueChanged<bool> onChanged;
  final Color? activeColor;

  @override
  Widget build(BuildContext context) {
    return CupertinoSwitch(
      value: value,
      activeTrackColor: activeColor ?? PlatformColors.primary,
      onChanged: onChanged,
    );
  }
}

// ---------------------------------------------------------------------------
// PlatformActivityIndicator
// ---------------------------------------------------------------------------

/// A platform-appropriate loading spinner.
class PlatformActivityIndicator extends StatelessWidget {
  const PlatformActivityIndicator({super.key});

  @override
  Widget build(BuildContext context) {
    return const CupertinoActivityIndicator();
  }
}

// ---------------------------------------------------------------------------
// PlatformNavButton
// ---------------------------------------------------------------------------

/// A navigation-bar button with zero padding.
class PlatformNavButton extends StatelessWidget {
  const PlatformNavButton({
    super.key,
    required this.icon,
    required this.onPressed,
    this.color,
    this.size = 22,
  });

  final IconData icon;
  final VoidCallback? onPressed;
  final Color? color;
  final double size;

  @override
  Widget build(BuildContext context) {
    return CupertinoButton(
      padding: EdgeInsets.zero,
      onPressed: onPressed,
      child: Icon(icon, color: color ?? PlatformColors.primary, size: size),
    );
  }
}

// ---------------------------------------------------------------------------
// showPlatformActionSheet
// ---------------------------------------------------------------------------

/// Shows a modal action sheet.
void showPlatformActionSheet({
  required BuildContext context,
  String? title,
  required List<PlatformSheetAction> actions,
  PlatformSheetAction? cancelAction,
}) {
  showCupertinoModalPopup<void>(
    context: context,
    builder: (ctx) => CupertinoActionSheet(
      title: title != null ? Text(title) : null,
      actions: [
        for (final action in actions)
          CupertinoActionSheetAction(
            isDefaultAction: action.isDefault,
            isDestructiveAction: action.isDestructive,
            onPressed: () {
              action.onPressed();
              Navigator.pop(ctx);
            },
            child: Text(action.label),
          ),
      ],
      cancelButton: cancelAction != null
          ? CupertinoActionSheetAction(
              onPressed: () {
                cancelAction.onPressed();
                Navigator.pop(ctx);
              },
              child: Text(cancelAction.label),
            )
          : null,
    ),
  );
}

/// Descriptor for a single action sheet item.
class PlatformSheetAction {
  const PlatformSheetAction({
    required this.label,
    required this.onPressed,
    this.isDefault = false,
    this.isDestructive = false,
  });

  final String label;
  final VoidCallback onPressed;
  final bool isDefault;
  final bool isDestructive;
}

// ---------------------------------------------------------------------------
// PlatformIcons
// ---------------------------------------------------------------------------

/// Consistent icon mapping across platforms.
class PlatformIcons {
  PlatformIcons._();

  static IconData get back => CupertinoIcons.back;
  static IconData get send => CupertinoIcons.arrow_up;
  static IconData get search => CupertinoIcons.search;
  static IconData get music => CupertinoIcons.music_note_2;
  static IconData get musicNote => CupertinoIcons.music_note;
  static IconData get person => CupertinoIcons.person_fill;
  static IconData get swap => CupertinoIcons.arrow_right_arrow_left;
  static IconData get logout => CupertinoIcons.square_arrow_right;
  static IconData get lock => CupertinoIcons.lock_fill;
  static IconData get play => CupertinoIcons.play_fill;
}

// ---------------------------------------------------------------------------
// PlatformColors
// ---------------------------------------------------------------------------

/// Consistent colour palette across platforms.
class PlatformColors {
  PlatformColors._();

  static Color get primary => CupertinoColors.systemBlue;
  static Color get destructive => CupertinoColors.systemRed;
  static Color get text => CupertinoColors.white;
  static Color get secondaryText => CupertinoColors.systemGrey;
  static Color get surface => const Color(0xFF1C1C1E);
  static Color get background => const Color(0xFF000000);
  static Color get separator => const Color(0xFF2C2C2E);
  static Color get selectedSurface =>
      CupertinoColors.systemBlue.withValues(alpha: 0.3);

  // Semantic colours used by specific widgets
  static Color get teal => CupertinoColors.systemTeal;
  static Color get orange => CupertinoColors.systemOrange;
  static Color get red => CupertinoColors.systemRed;
  static Color get green => CupertinoColors.systemGreen;
  static Color get yellow => CupertinoColors.systemYellow;
  static Color get purple => CupertinoColors.systemPurple;
  static Color get blue => CupertinoColors.systemBlue;

  /// The darker surface used for borders/outlines.
  static Color get outline => const Color(0xFF3A3A3C);
}

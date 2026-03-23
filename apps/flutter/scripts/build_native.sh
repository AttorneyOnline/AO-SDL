#!/bin/bash
# Build the native C++ bridge library for Flutter.
#
# Usage:
#   ./scripts/build_native.sh [sim|device]
#
# This script:
#   1. Configures + builds the C++ static libraries via CMake
#   2. Creates the symlink that the CocoaPods podspec references
#   3. Runs pod install to wire everything into the Xcode project
#
# After running this, 'flutter run' will link the native bridge.

set -euo pipefail

REPO_ROOT="$(cd "$(dirname "$0")/../../.." && pwd)"
FLUTTER_DIR="$REPO_ROOT/apps/flutter"
TARGET="${1:-sim}"

case "$TARGET" in
    sim)
        PRESET="ios-sim-debug"
        ;;
    device)
        PRESET="ios-device-release"
        ;;
    *)
        echo "Usage: $0 [sim|device]"
        exit 1
        ;;
esac

echo "==> Configuring: cmake --preset $PRESET"
cmake --preset "$PRESET" -S "$REPO_ROOT"

echo "==> Building: ao_bridge + dependencies"
cmake --build "$REPO_ROOT/out/build/$PRESET" --target ao_bridge -j "$(sysctl -n hw.ncpu)"

echo "==> Linking build output into iOS project"
mkdir -p "$FLUTTER_DIR/ios/libs"
ln -sfn "$REPO_ROOT/out/build/$PRESET" "$FLUTTER_DIR/ios/libs/ios-sim"

echo "==> Running pod install"
cd "$FLUTTER_DIR/ios" && pod install

echo ""
echo "==> Done. You can now run: cd apps/flutter && flutter run -d iPhone"

# AO SDL
## An Attorney Online client, implemented from base principles

AO SDL is a reimplementation of AO2 using SDL2, OpenGL, and imgui. Focus is on correctness, performance, and modularized code.

See [Architecture.md](doc/Architecture.md) for a detailed overview of the design principles, component architecture, and threading model. See [STYLE.md](STYLE.md) for coding conventions and naming rules.

## Building

### Prerequisites

| | macOS | Linux (Ubuntu/Debian) | Windows |
|---|---|---|---|
| **Compiler** | Xcode command line tools | GCC or Clang with C++20 | MSVC (Visual Studio 2019+) |
| **Build tools** | CMake, Ninja | CMake, Ninja or Make | CMake, Ninja (via VS) |
| **Graphics** | Metal (system) | `libglew-dev`, OpenGL drivers | Bundled GLEW in `third-party/` |
| **SSL (optional)** | `brew install openssl` | `libssl-dev` | Install OpenSSL, set `OPENSSL_ROOT_DIR` |
| **Other** | — | `libsdl2-dev` (optional, also built from source) | — |

OpenSSL is optional — without it, HTTPS asset fetching is disabled and only HTTP connections will work.

### Clone and Initialize Submodules

```sh
git clone --recursive <repo-url>
cd AO-SDL
```

If you already cloned without `--recursive`:

```sh
git submodule update --init --recursive
```

### Configure and Build

**macOS:**
```sh
cmake --preset macos-debug
cmake --build build
```

**Linux:**
```sh
# Install system dependencies (Ubuntu/Debian)
sudo apt install cmake ninja-build libglew-dev libssl-dev

cmake --preset linux-debug
cmake --build out/build/linux-debug
```

**Windows (Developer Command Prompt or PowerShell with VS environment):**
```sh
cmake --preset x64-debug
cmake --build out/build/x64-debug
```

Available presets: `x64-debug`, `x64-release`, `x86-debug`, `x86-release`, `linux-debug`, `macos-debug`.

### Run Tests

```sh
# From the build directory
cd <build-dir>/tests
./aosdl_tests
```

### Git Hooks

The project includes a pre-commit hook that auto-formats staged C++/ObjC files with `clang-format`. To enable it:

```sh
git config core.hooksPath .githooks
```

Install `clang-format` via your package manager (`brew install clang-format`, `apt install clang-format`, etc.). If `clang-format` is not found, the hook is skipped with a warning.

### Optional Targets

- **`docs`** — Generate API documentation with Doxygen (requires `doxygen` installed)
- **`run-clang-tidy`** — Run clang-tidy static analysis across the project (requires `clang-tidy` installed)

---

## Flutter Mobile App (iOS / Android)

The `apps/flutter/` directory contains a cross-platform mobile client that reuses the C++ engine, game logic, and Metal renderer via `dart:ffi`. The Flutter UI replaces SDL/ImGui with native Cupertino widgets on iOS.

### Prerequisites

| Tool | Install |
|---|---|
| Flutter SDK | `brew install --cask flutter` |
| Xcode (full, not just CLI tools) | App Store |
| iOS Simulator runtime | `xcodebuild -downloadPlatform iOS` |
| CocoaPods | `brew install cocoapods` |

After installing Xcode:
```sh
sudo xcode-select -s /Applications/Xcode.app/Contents/Developer
sudo xcodebuild -license accept
```

### Build and Run

**1. Build the native C++ bridge for iOS Simulator:**
```sh
cmake --preset ios-sim-debug
cmake --build out/build/ios-sim-debug --target flutter-build
```

**2. Set up the iOS project (first time only):**
```sh
cd apps/flutter
./scripts/build_native.sh sim
```
This configures CMake, builds the native libs, creates the symlink, and runs `pod install`.

**3. Run on the iOS Simulator:**
```sh
cd apps/flutter
flutter run -d iPhone
```

**For a real device:**
```sh
cmake --preset ios-device-release
cmake --build out/build/ios-device-release --target flutter-build
cd apps/flutter && flutter run -d <device-name>
```

### Available CMake Presets

| Preset | Target | Use |
|---|---|---|
| `macos-debug` | macOS desktop (SDL) | Desktop development |
| `ios-sim-debug` | iOS Simulator (Flutter) | Mobile development |
| `ios-device-release` | iOS device (Flutter) | Device testing / release |

The `AO_BUILD_FLUTTER` flag (set automatically by the iOS presets) excludes SDL, ImGui, and tests from the build, and includes the Flutter FFI bridge, miniaudio audio backend, and NSURLSession HTTPS shim.

### VS Code Integration

Open the repo root in VS Code and install the **Flutter** and **Dart** extensions.

**Launch configurations** (`.vscode/launch.json`):

| Config | What it does |
|---|---|
| **Debug aosdl** | Build and run the desktop SDL app |
| **Flutter (iOS Sim)** | Build native C++ bridge, then launch Flutter on the simulator |
| **Flutter (iOS Sim, skip native)** | Launch Flutter without rebuilding C++ (Dart-only changes) |
| **Flutter (iOS Device)** | Build release native bridge, then launch on a real device |

Press **F5** and select a configuration from the dropdown. "Flutter (iOS Sim)" builds everything from scratch on first run; subsequent runs are incremental.

**Build tasks** (`.vscode/tasks.json`):

| Task | What it does |
|---|---|
| **CMake: Build (Debug)** | Build the desktop SDL app (default build task, `Cmd+Shift+B`) |
| **Flutter: Build Native Bridge (iOS Sim)** | Build the C++ bridge for iOS Simulator |

### Architecture

The Flutter app mirrors `apps/sdl/` with a clean separation:

```
apps/flutter/
├── native/                  # C bridge layer
│   ├── bridge.h / .cpp      # C API (50+ functions) wrapping engine/plugins
│   ├── MiniaudioDevice.*    # IAudioDevice using miniaudio (CoreAudio/AAudio)
│   ├── apple_httplib.*      # NSURLSession-based httplib replacement
│   └── miniaudio_device_impl.mm  # miniaudio device I/O (Obj-C++)
├── lib/
│   ├── bridge/              # Dart FFI bindings
│   ├── screens/             # Server list, character select, courtroom
│   ├── widgets/             # IC chat, emotes, music, etc.
│   └── widgets/platform/    # Platform widget abstraction (Cupertino ↔ Material)
├── ios/
│   ├── Runner/AoTexturePlugin.*  # FlutterTexture for Metal render output
│   ├── ao_native.xcconfig   # Links pre-built static libs into Runner
│   └── ao_bridge.podspec    # CocoaPods integration
└── scripts/
    └── build_native.sh      # One-command build + symlink + pod install
```

**Key design decisions:**
- The C++ engine (`aoengine`, `ao_game`, `ao_net`, `aorender_metal`) is compiled as static libraries and linked into the Flutter runner via xcconfig
- `bridge.cpp` provides a flat C API that Dart calls via `dart:ffi` — no Dart ↔ C++ object sharing
- The Metal renderer draws to an offscreen texture; `AoTexturePlugin` blits it to a `CVPixelBuffer` for Flutter's `Texture` widget
- Audio uses miniaudio's CoreAudio backend (iOS) instead of SDL
- HTTPS uses `NSURLSession` via a drop-in `httplib::Client` shim (no OpenSSL dependency on iOS)
- Platform widgets are abstracted so the same screens work with Cupertino (iOS) or Material (Android) by swapping one file

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
| **TLS** | Secure Transport (system) | `libssl-dev` | Install OpenSSL, set `OPENSSL_ROOT_DIR` |
| **Other** | — | `libsdl2-dev` (optional, also built from source) | — |

**TLS is required** on Linux and Windows (OpenSSL). On macOS/iOS, the platform's Secure Transport framework is used instead — no extra dependencies needed.

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

Available presets: `x64-debug`, `x64-release`, `x86-debug`, `x86-release`, `linux-debug`, `macos-debug`, `macos-release`.

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

### Schema Generation (Optional)

The AONX REST API validates request bodies against JSON schemas generated from the OpenAPI spec (`doc/aonx/openapi.yaml`). This is **enabled by default**.

**Requirements:** Python 3 + [PyYAML](https://pypi.org/project/PyYAML/) (`pip install pyyaml`).

CMake runs `scripts/generate_schemas.py` at build time to produce `AonxSchemas.cpp` from the OpenAPI spec. The generated file is compiled into `nx_net` and defines `AOSDL_HAS_GENERATED_SCHEMAS=1`. Endpoints validate request bodies against the spec at runtime.

To disable (e.g. if Python is unavailable):

```sh
cmake -B build -DAOSDL_GENERATE_SCHEMAS=OFF
```

Without schema generation, endpoints log an ERROR on every request but do not validate bodies. If the flag is ON (default) but Python or PyYAML is missing, CMake will fail with an error directing you to install the dependencies or pass `-DAOSDL_GENERATE_SCHEMAS=OFF`.

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
| `macos-release` | macOS desktop (SDL) | Release build (-Os, LTO, strip) |
| `ios-sim-debug` | iOS Simulator (Flutter) | Mobile development |
| `ios-device-release` | iOS device (Flutter) | Device testing / release |

The `AO_BUILD_FLUTTER` flag (set automatically by the iOS presets) excludes SDL, ImGui, and tests from the build, and includes the Flutter FFI bridge and miniaudio audio backend.

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
- TLS uses Apple's Secure Transport via the platform socket abstraction (no OpenSSL dependency on iOS)
- Platform widgets are abstracted so the same screens work with Cupertino (iOS) or Material (Android) by swapping one file

---

## Kagami Server

The `apps/kagami/` directory contains **Kagami**, a standalone multi-protocol game server for Attorney Online. It handles player sessions, character selection, area management, and message routing for both legacy AO2 and next-generation AONX clients simultaneously.

### Building

Kagami is built alongside the desktop client by default:

```sh
cmake --preset macos-debug    # or linux-debug, x64-debug
cmake --build build --target kagami
```

The binary is output to `build/apps/kagami/kagami`.

### Running

```sh
./kagami
```

On first run, Kagami creates a `kagami.json` config file next to the binary with sensible defaults. The server runs in interactive mode when launched from a terminal, providing a REPL with commands like `/status`, `/stop`, and `/help`.

### Configuration

All settings are stored in `kagami.json` and can be edited while the server is stopped:

| Setting | Default | Description |
|---|---|---|
| `server_name` | `"Kagami Server"` | Display name shown to clients |
| `server_description` | `""` | Server description |
| `bind_address` | `"0.0.0.0"` | Listen address |
| `http_port` | `8080` | HTTP status endpoint port |
| `ws_port` | `8081` | WebSocket game port |
| `max_players` | `100` | Maximum concurrent players |
| `motd` | `""` | Message of the day |
| `session_ttl_seconds` | `300` | REST session TTL in seconds (0 = no expiry) |
| `cors_origin` | `"https://web.aceattorneyonline.com"` | CORS allowed origin(s) — string, `"*"`, or array of strings |

### Deploying

The `deploy/` directory contains everything needed to run Kagami in production.

#### Quick start

```sh
cd deploy
cp kagami.example.json kagami.json   # edit with your server settings
KAGAMI_DOMAIN=my.server.com docker compose up -d
```

**Prerequisites:** A Linux server (arm64 or amd64) with Docker and Docker Compose. Run `deploy/bootstrap.sh` on a fresh Ubuntu instance to install them. Point your domain's DNS to the server's IP — Caddy handles TLS certificates automatically via Let's Encrypt.

#### Full stack (observability)

The default `docker-compose.yml` runs 5 services:

| Service | Purpose | URL | RAM |
|---|---|---|---|
| **Caddy** | TLS + reverse proxy | `https://your.domain/` | ~18 MB |
| **Kagami** | Game server (AO2 + AONX) | Port 27015 (WebSocket, direct) | ~20 MB |
| **Prometheus** | Metrics collection | `https://your.domain/prometheus/` | ~100 MB |
| **Loki** | Log aggregation | (internal, queried via Grafana) | ~80 MB |
| **Grafana** | Dashboards + log viewer | `https://your.domain/grafana/` | ~130 MB |

This requires ~350 MB of RAM for the full stack. A t4g.micro (1 GB) or equivalent is recommended.

Optionally create a `.env` file with `GRAFANA_ADMIN_PASSWORD=your_password` (defaults to `kagami`). Anonymous visitors get read-only dashboard access. A 34-panel Grafana dashboard is auto-provisioned on first boot — no manual setup needed.

#### Minimal stack (game server only)

If you don't need observability, remove the `prometheus`, `loki`, and `grafana` services from `docker-compose.yml` and trim the `Caddyfile` to just proxy to `kagami:80`. This runs on ~40 MB total — a t4g.nano (512 MB) is more than enough.

Kagami's `/metrics` endpoint still works in the minimal setup, so you can add monitoring later or scrape remotely from another server.

#### Updating

On pushes to `master`, CI automatically builds the Docker image and pushes it to GHCR. If you set the `DEPLOY_SSH_KEY` and `DEPLOY_HOST` secrets in your fork, CI will also deploy to your server:

```sh
cd /opt/kagami && docker compose pull kagami && docker compose up -d kagami
```

Only the `kagami` container restarts — Caddy, Prometheus, Grafana, and Loki continue running.

For manual deploys (e.g., testing a branch), build and push the image locally:

```sh
docker buildx build --platform linux/arm64 \
  -t ghcr.io/attorneyonline/kagami:latest \
  --output type=docker,dest=/tmp/kagami.tar .

scp -i your-key.pem /tmp/kagami.tar user@your.server:/tmp/
ssh -i your-key.pem user@your.server "
  docker load < /tmp/kagami.tar
  cd /opt/kagami && docker compose up -d kagami
"
```

#### Observability

Kagami exposes a Prometheus-compatible `/metrics` endpoint with 33 metric families covering network I/O, sessions, areas, lock contention, memory, and per-client traffic. Logs are pushed to Loki (if configured) with Grafana-native log levels for filtered queries. See `include/metrics/` for the full metric inventory.

### Architecture

Kagami uses a protocol-agnostic core with pluggable protocol backends:

```
apps/kagami/              # Server application
├── main.cpp              # Entry point, wiring, REPL
├── ServerSettings.*      # kagami.json configuration
└── TerminalUI.*          # Interactive terminal with log display

plugins/kagami_server/    # Protocol layer (engine plugin)
├── kagami/
│   ├── ProtocolRouter.*  # Routes clients by WebSocket subprotocol
│   ├── AOServer.*        # AO2 backend (#%-delimited packets)
│   └── NXServer.*        # AONX backend (JSON messages)
└── game/
    ├── GameRoom.*        # Authoritative game state
    ├── GameAction.h      # Protocol-agnostic input actions
    ├── GameEvent.h       # Protocol-agnostic output events
    └── ServerSession.*   # Per-player session state
```

**Key design decisions:**
- **Multi-protocol**: A `ProtocolRouter` inspects the WebSocket subprotocol header (`ao2` or `aonx`) and routes each client to the correct backend — both protocol types can play in the same room
- **Action/Event model**: Protocol backends parse wire formats into protocol-agnostic `GameAction`s; the `GameRoom` validates and processes them, then emits `GameEvent`s that each backend serializes back to its own wire format
- **Transport**: WebSocket (RFC 6455) for game traffic; event-loop HTTP server for REST API and Server-Sent Events (SSE)

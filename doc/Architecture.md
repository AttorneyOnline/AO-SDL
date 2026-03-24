# Architecture

AO-SDL is a modern reimplementation of the Attorney Online 2 client built on SDL2, Dear ImGui, and OpenGL/Metal. The architecture prioritizes modularity, testability, and cross-platform support through strict separation between engine infrastructure, game-specific logic, and platform backends.

---

## Design Principles

### Interface-Driven Boundaries

Every major subsystem is defined by an abstract interface in `include/`. Plugins and backends implement these interfaces without the engine knowing about them. This means the engine never depends on a specific renderer, protocol, or game — only on the contracts those systems fulfill.

Key interfaces:
- **IRenderer** — GPU rendering backend (OpenGL, Metal)
- **IScenePresenter** — Game logic that produces renderable frames
- **ProtocolHandler** — Network protocol implementation
- **ITcpSocket** — TCP transport (real or mock)
- **Mount** — Virtual filesystem backend (disk, archive, HTTP, embedded)

### Thread Isolation via Message Passing

The three primary threads (network, game, render) never share mutable state directly. Communication happens through two mechanisms:

- **EventChannel\<T\>** — Type-safe FIFO queues for discrete events (network packets → game state changes → UI updates)
- **StateBuffer** — Triple-buffered snapshot exchange for continuous frame data (game thread produces RenderState, render thread consumes it without blocking)

This eliminates the need for fine-grained locking in game logic or rendering code.

### Layered Plugin Architecture

The codebase is organized into four layers, each with strict dependency rules:

```
┌─────────────────────────────────────────────┐
│  App Layer (apps/sdl/)                      │
│  SDL window, ImGui widgets, controllers     │
│  Depends on: Engine, Plugins (via factory)  │
├─────────────────────────────────────────────┤
│  Plugin Layer (plugins/)                    │
│  AO game logic, GL/Metal renderers, AO net  │
│  Depends on: Engine interfaces only         │
├─────────────────────────────────────────────┤
│  Engine Layer (engine/, include/)           │
│  Assets, events, rendering, networking      │
│  Depends on: Nothing above                  │
├─────────────────────────────────────────────┤
│  Platform Layer (platform/)                 │
│  System fonts, hardware ID                  │
│  Depends on: OS APIs only                   │
└─────────────────────────────────────────────┘
```

Plugins only link against engine interfaces. The engine has zero knowledge of any plugin. The app layer wires everything together at startup through factory functions exposed by each plugin.

### Cache-Friendly Asset Pipeline

Assets flow through a three-stage pipeline designed to minimize redundant I/O and decoding:

1. **MountManager** fetches raw bytes from the first available source
2. **Decoders** transform bytes into usable data (images, configs, shaders)
3. **AssetCache** stores decoded assets with LRU eviction and shared_ptr pinning

Callers that hold a `shared_ptr<Asset>` prevent eviction. Once all references are released, the asset becomes eligible for eviction under memory pressure.

### Minimal Platform Surface

Platform-specific code is confined to `platform/` and limited to two concerns: system font discovery and hardware identification. Everything else — windowing, input, rendering, networking — goes through portable libraries (SDL2, kissnet, cpp-httplib).

---

## Core Components

### Event System

**Files:** `include/event/EventManager.h`, `include/event/EventChannel.h`

The event system provides decoupled communication between threads and subsystems.

- **EventManager** is a singleton registry that maps `std::type_index` → `EventChannel<T>`. Channels are created lazily on first access.
- **EventChannel\<T\>** is a mutex-protected FIFO queue. Publishers move events in; consumers poll with `get_event()` returning `std::optional<T>`.
- Events are plain structs (e.g., `ICMessageEvent`, `ChatEvent`, `TimerEvent`). They carry data, not behavior.

Typical flow: the network thread publishes an `ICMessageEvent` → the game thread consumes it and updates scene state → the game thread produces a `RenderState` → the render thread draws it.

### Asset System

**Files:** `include/asset/AssetLibrary.h`, `include/asset/MountManager.h`, `include/asset/AssetCache.h`

#### MountManager (Virtual Filesystem)

MountManager aggregates multiple `Mount` backends searched in priority order:

| Mount Type | Source | Use Case |
|---|---|---|
| MountDirectory | Local filesystem | Development, user content |
| MountArchive | 7z/zip archives | Distributed asset packs |
| MountEmbedded | Compiled into binary | Fallback resources |
| MountHttp | Remote HTTP server | On-demand asset streaming |

Thread safety: `std::shared_mutex` allows concurrent reads with exclusive writes when mounts are added/removed.

#### AssetLibrary (Format Resolution)

AssetLibrary handles format probing — when loading an image, it tries extensions in preference order (webp → apng → gif → png) against the mount system. This allows assets to be upgraded to better formats without changing any calling code.

Key entry points:
- `image(path)` — Load with format probing and animation support
- `shader(path)` — Load vertex + fragment source for the active backend
- `config(path)` — Parse INI configuration files
- `prefetch_image(path)` — Trigger async HTTP download

#### AssetCache (LRU with Pinning)

Decoded assets are stored in an LRU cache with a soft memory limit. The eviction policy respects `shared_ptr` reference counts — assets actively in use (held by game or render code) are never evicted, even if they sit at the tail of the LRU list.

#### Image Decoders

A chain of decoders attempts to decode raw bytes in registration order:

1. **WebPDecoder** — Animated WebP (libwebp)
2. **ApngDecoder** — Animated PNG
3. **GifDecoder** — Animated GIF
4. **StbiDecoder** — Static formats (PNG, JPEG, BMP via stb_image)

Each decoder returns RGBA frames. The first successful decoder wins.

### Rendering

**Files:** `include/render/IRenderer.h`, `include/render/Layer.h`, `include/render/RenderManager.h`

#### RenderState and Layers

The game thread produces a `RenderState` each tick — an ordered map of `LayerGroup` instances, each containing `Layer` objects. A Layer holds:

- `ImageAsset` (texture data + frame index)
- `Transform` (position, rotation, scale, z-index)
- Optional `ShaderAsset` and `MeshAsset` for custom effects
- Opacity

LayerGroups can have their own shader and transform, applied to all contained layers (used for effects like screenshake).

#### StateBuffer (Triple Buffer)

StateBuffer provides lock-free frame exchange between the game and render threads using three buffer slots (preparing, ready, presenting). The game thread writes to "preparing" and swaps it to "ready"; the render thread picks up "ready" and presents it. Neither thread ever blocks the other.

#### IRenderer Implementations

- **GLRenderer** — OpenGL 4.5 with texture/mesh/shader caches keyed by asset pointer + generation counter. Supports wireframe debug mode.
- **MetalRenderer** — Apple Metal via Pimpl pattern. Provides device and command queue handles for ImGui Metal backend integration.

Both renderers draw to an offscreen framebuffer. The resulting texture is composited into the ImGui scene via `ImGui::Image()`.

#### Text Rendering

Text is prepared on the CPU and drawn on the GPU:

1. **TextRenderer** rasterizes glyphs via FreeType
2. **GlyphCache** packs glyphs into a texture atlas (row-based bin packing, max 2048px), pre-caching printable ASCII
3. **TextMeshBuilder** converts text layout into a `MeshAsset` with per-glyph quads and atlas UVs

The CPU handles glyph rasterization and mesh construction, producing a texture atlas and quad mesh that the GPU renders like any other layer. This keeps text preparation independent of the GPU backend.

#### Math and SIMD

`Math.h` provides `Vec2`, `Vec3`, and `Mat4` types with platform-selected SIMD acceleration (ARM NEON on Apple Silicon, SSE on x86-64, scalar fallback otherwise). `Transform` uses `Mat4` internally and recalculates on each operation.

### Networking

**Files:** `include/net/HttpPool.h`, `include/net/WebSocket.h`, `include/net/ProtocolHandler.h`

#### WebSocket

The WebSocket client operates over an injected `ITcpSocket` interface, enabling unit testing with `MockTcpSocket`. It handles the HTTP upgrade handshake, frame encoding/decoding, and connection state management per RFC 6455.

#### HttpPool

A thread pool for async HTTP GET requests with priority scheduling (CRITICAL, HIGH, NORMAL, LOW). Work items are sorted by priority. Results are queued and delivered to the main thread via `poll()`. Supports dropping queued requests below a priority threshold.

#### ProtocolHandler

The abstract protocol interface. `AOClient` implements it for the AO2 wire protocol:
- Deserializes packets via `PacketRegistrar` (factory pattern)
- Translates packets into engine events
- Manages connection lifecycle (NOT_CONNECTED → CONNECTED → JOINED)
- Sends keepalive heartbeats (45-second interval)

### Threading Model

```
┌────────────────────────────────┐
│ Main Thread                    │
│ SDL event loop + ImGui render  │
│ HTTP poll + UI controllers     │
│ Reads from StateBuffer         │
└───────────┬────────────────────┘
            │
    ┌───────┴───────┐
    │  StateBuffer   │ (triple-buffered RenderState)
    └───────┬───────┘
            │
┌───────────┴────────────────────┐
│ Game Thread (~10 Hz)           │
│ IScenePresenter::tick()        │
│ Consumes events, produces      │
│ RenderState, writes to         │
│ StateBuffer                    │
└───────────┬────────────────────┘
            │
    ┌───────┴───────┐
    │ EventManager   │ (typed FIFO channels)
    └───────┬───────┘
            │
┌───────────┴────────────────────┐
│ Network Thread                 │
│ ProtocolHandler::poll()        │
│ Publishes events from packets  │
└────────────────────────────────┘
```

Each thread has a clear role and communicates only through the defined exchange mechanisms. No raw shared mutable state.

### UI Architecture

**Files:** `apps/sdl/ui/`

The UI follows a **Screen → Controller → Widget** hierarchy:

- **Screen** (in plugins) — Represents game state (server list, character select, courtroom). Managed as a stack by `UIManager`.
- **IScreenController** (in app) — Bridges a Screen to ImGui rendering. Created by a factory in `ImGuiUIRenderer` based on screen ID.
- **IWidget** (in app) — Individual ImGui components (`ChatWidget`, `CourtroomWidget`, `ICChatWidget`, etc.). Each widget implements `handle_events()` and `render()`.

Controllers own their widgets and orchestrate layout. Widgets read state from events and screens, and publish outgoing events for user actions. Navigation is handled by controllers returning `NavAction` values (POP_SCREEN, POP_TO_ROOT) to the UI manager.

---

## AO Game Plugin

**Files:** `plugins/ao/`

The AO plugin implements Attorney Online 2 game logic without depending on any specific UI toolkit or renderer.

### AOCourtroomPresenter

The central game logic class, implementing `IScenePresenter`. Called by `GameThread` at ~10 Hz. Each tick:

1. Consumes events from EventManager (IC messages, background changes, etc.)
2. Advances animation state (emote player, textbox, effects)
3. Composes a `RenderState` from active layers
4. Publishes the state to `StateBuffer`

Provides per-section tick profiling (events, assets, animation, textbox, effects, compose).

### AOAssetLibrary

Encapsulates all AO2 path conventions — character directories, background folders, theme chains, emote prefixes. Translates logical AO2 asset names into engine `AssetLibrary` calls. Handles HTTP prefetching for on-demand asset streaming.

### Game Components

- **AOEmotePlayer** — State machine driving preanim → talking → idle animation sequences
- **AOTextBox** — Courtroom chatbox with per-character text reveal timing, showname rendering, and background image management
- **ICMessageQueue** — Sequences incoming IC messages; objections interrupt the queue
- **AOBackground** — Background and desk overlay state management

### Scene Effects

Effects modify `LayerGroup` state each tick:

- **ScreenshakeEffect** — Transform-based screen shake
- **FlashEffect** — White flash overlay
- **ShaderEffect** — Time-driven GPU shader effects (rainbow, shatter, cube)

---

## Build System

**CMake 3.14+** with C++20. Presets for Windows (x64/x86), Linux, and macOS.

### Targets

| Target | Type | Description |
|---|---|---|
| `aoengine` | Static library | Core engine |
| `ao_game` | Static library | AO game logic plugin |
| `ao_net` | Static library | AO network protocol plugin |
| `aorender_gl` | Static library | OpenGL renderer (all platforms) |
| `aorender_metal` | Static library | Metal renderer (Apple only) |
| `aosdl` | Executable | SDL application |
| `aosdl_tests` | Executable | Google Test suite |

### Key Build Features

- **Asset embedding** — Files in `assets/` are compiled into the binary via `EmbedAssets.cmake`
- **Version string** — Auto-generated from git (commit hash + dirty flag)
- **Compile commands** — Exported for IDE integration and static analysis
- **Platform selection** — Metal renderer built only on Apple; GL renderer available everywhere

### Third-Party Dependencies

| Library | Purpose |
|---|---|
| SDL2 | Windowing, input, platform abstraction |
| Dear ImGui | UI toolkit |
| FreeType | Font rasterization |
| libwebp | WebP image codec |
| bit7z | 7z archive handling |
| kissnet | TCP sockets |
| stb_image | Static image loading |
| GLEW | OpenGL extension loading |
| cpp-httplib | HTTP client |
| nlohmann/json | JSON parsing |
| Google Test | Unit testing framework |

---

## Testing

Tests use Google Test 1.14.0 (fetched via CMake FetchContent) and cover:

- **Protocol:** Packet parsing, serialization, wire format compliance
- **Game logic:** Emote player state machine, textbox behavior, background loading
- **Events:** Event channel publish/consume, IC message events
- **Assets:** Cache eviction, APNG decoding
- **Rendering:** Animation player, layer composition, text mesh building
- **Networking:** WebSocket handshake and framing (via MockTcpSocket)
- **Utilities:** Base64, UTF-8, JSON validation

The `MockTcpSocket` test double enables WebSocket testing without network access, demonstrating the value of the `ITcpSocket` abstraction.

# Qt Frontend — Current Status

**Branch:** `qt-frontend`
**Last updated:** 2026-03-27
**Migration phase:** Phase 3 complete

---

## What is done

### Phase 1 — GL game renderer in the PoC ✓
- `SceneTextureItem` replaced the triangle PoC with `RenderManager`-driven rendering
- `RenderBridge` extended with `setStateBuffer()` / `setRenderManager()` so the render thread can initialise `RenderManager` from `initGL()`
- `GLRenderer::draw()` leaves its own FBO bound → explicit `glBindFramebuffer(GL_FRAMEBUFFER, 0)` added in `SceneTextureItem::renderGL()` after `renderFrame()`
- `GameThread` + `StateBuffer` wired in `main.cpp`; `NullScenePresenter` (now replaced) provided animated RGB cycling to verify the pipeline end-to-end

### Phase 2 — Engine event loop ✓
- `EngineEventBridge` connects to `QAbstractEventDispatcher::awake` so drain lambdas fire on every Qt event-loop wakeup
- `UIManager` owns the screen stack; initial `ServerListScreen` push in `main.cpp`
- `HttpPool` (4 threads) + master-server list fetch on startup
- `NetworkThread` driven by `ao::create_protocol()`
- `Session` lifecycle: `SessionStartEvent` → create session + add fallback HTTP mount; `AssetUrlEvent` → add server mount; `SessionEndEvent` → destroy session
- `QtAppInterface` singleton owns UIManager, controllers, and EngineEventBridge; exposes controllers + `currentScreenId` Q_PROPERTY to QML; `syncCurrentScreenId()` drains UIManager → emits change signal

### Phase 3 — Event-driven controllers, UIManager as authority ✓
- **`IQtScreenController`**: `sync(Screen&)` → `drain()` (no Screen parameter); `navActionRequested` signal removed; controllers navigate via `UIManager` directly
- **`ServerListController`**: `drain()` consumes `ServerListEvent`, mirrors `m_entries`; `connectToServer()` publishes `ServerConnectEvent` + pushes `CharSelectScreen`; `directConnect()` same path
- **`CharSelectController`**: `drain()` consumes `CharacterListEvent` (rebuild roster), `CharsCheckEvent` (update taken flags), `UIEvent::ENTERED_COURTROOM` (inject char name into `CourtroomController`, push `CourtroomScreen`); HTTP icon prefetch runs in `drain()` at 32 prefetch + retry pass per tick; holds `CourtroomController&` for char-name injection
- **`CourtroomController`**: `drain()` pulls all event channels directly (chat, players, evidence, music, health, now-playing); `setInitialCharName()` called by CharSelectController before courtroom push; `disconnect()` calls `reset()` + `m_uiMgr.pop_to_root()`
- **`CharListModel`**: extracted from the old `CharSelectController.h` into `ui/models/CharListModel.h/.cpp`
- **`ao::create_presenter()`** replaces `NullScenePresenter` — real AO2 courtroom renderer active
- `QtUIRenderer` is NOT used or compiled — deleted from the active build

---

## File map

```
apps/qt/
├── main.cpp                          Phase 3 wiring — all controllers, bridge, ao::create_presenter()
├── CMakeLists.txt                    All sources + QML files registered
├── EngineInterface.h/.cpp            Pure C++ — owns engine services (HttpPool, GameThread, net, session)
├── QtAppInterface.h/.cpp             QObject — owns UIManager, controllers, EngineEventBridge; QML "app" context property
├── EngineEventBridge.h/.cpp          Qt event-loop → drain lambda dispatcher
├── NullScenePresenter.h              Kept but no longer used in main; RGB cycling stub
│
├── render/
│   ├── RenderBridge.h/.cpp           StateBuffer/RenderManager handoff; render-thread init
│   ├── SceneTextureItem.h/.cpp       QML element; initGL() → RenderManager; renderGL() → renderFrame()
│   └── (SceneTextureItem.cpp fixes:  glBindFramebuffer(0) after renderFrame())
│
├── ui/
│   ├── controllers/
│   │   ├── IQtScreenController.h     Base: drain() pure virtual; no Screen, no nav signal
│   │   ├── ServerListController.h/.cpp  drain=ServerListEvent; connectToServer→ServerConnectEvent+push
│   │   ├── CharSelectController.h/.cpp  drain=CharList+CharsCheck+UIEvent; prefetch; pushCourtroom
│   │   ├── CourtroomController.h/.cpp   drain=all courtroom events; setInitialCharName(); disconnect→pop_to_root
│   │   └── IQtScreenController.h        (see above)
│   │
│   ├── models/
│   │   ├── CharListModel.h/.cpp      New — char roster model (name, taken)
│   │   ├── ServerListModel.h/.cpp    Server browser model (name, hostname, description, players)
│   │   ├── ChatModel.h/.cpp          OOC chat log (sender, message, isSystem)
│   │   ├── EvidenceModel.h/.cpp
│   │   ├── MusicAreaModel.h/.cpp
│   │   └── PlayerListModel.h/.cpp
│   │
│   └── screens/
│
└── qml/
    ├── Main.qml                      ApplicationWindow + SceneTextureItem + Loader on currentScreenId
    ├── ServerListScreen.qml          ListView + direct-connect field
    ├── CharSelectScreen.qml          GridView of chars (name+taken); selectCharacter on click
    ├── CourtroomScreen.qml           Top HUD bar + right OOC chat panel
```

---

## Architecture invariants

| Rule | Where enforced |
|------|---------------|
| UIManager is the only navigation authority | Controllers hold `UIManager&`; call `push_screen` / `pop_screen` / `pop_to_root` directly |
| No `UIManager::handle_events()` called | EngineEventBridge drains controllers, not UIManager |
| No `Screen` as data source | Controllers consume EventChannels directly; Screens are navigation markers only |
| `currentScreenId` drives QML Loader | `QtAppInterface::syncCurrentScreenId()` drains UIManager → emits Q_PROPERTY change signal |
| GL work stays on render thread | `SceneTextureItem::initGL()` / `renderGL()` run on Qt's render thread; controllers on main thread |
| No `QtUIRenderer` | Deleted |

---

## Known deferred work (Phase 4)

- **Character icons in QML** — `CharSelectController::prefetchIcons()` fills the `AssetLibrary` HTTP cache but does no GPU upload (Texture2D creation requires the render thread). A `QQuickImageProvider` is needed to bridge asset cache → QML `Image` source.
- **Audio** — `AudioThread` + audio device not wired; `ao::create_presenter()` will play music once AudioThread is added.
- **Courtroom HUD** — health bars, emote grid, IC textbox, evidence panel not exposed to QML.
- **`CharSelectController::selectCharacter()` double-click shortcut** — re-selecting the same character pushes CourtroomScreen directly (matches SDL behavior); verify server expects the extra round-trip or not.
- **Window polish** — title, resize constraints, DPI scaling, discrete-GPU preference (already done for Windows via `gpu_preference.cpp`).
- **Font embedding** — NotoEmoji from embedded assets.

---

## Deferred — GPU backend abstraction layer

### What the SDL frontend has

`apps/sdl/render/IGPUBackend.h` defines an abstract interface that decouples the
GPU context lifecycle from the rest of the SDL application:

```
IGPUBackend
  window_flags()      — SDL window creation flags (SDL_WINDOW_OPENGL, etc.)
  pre_init()          — GL attributes that must be set before window creation
  create_context()    — creates the GL/Metal context and calls glewInit
  init_imgui()        — wires the ImGui rendering backend to the context
  begin_frame()       — ImGui NewFrame + acquire drawable
  present()           — submit ImGui draw data + swap buffers / present
  shutdown()          — ImGui shutdown + context teardown

create_gpu_backend()  — link-time factory (GLBackend.cpp on non-Apple)
```

`GLBackend` (in `apps/sdl/render/GLBackend.cpp`) is the concrete OpenGL 3.3
implementation. It owns the `SDL_GLContext`, sets the swap interval, and drives
`imgui_impl_opengl3`. Swapping in a `MetalBackend` at link time gives macOS
support with zero changes to the rest of the SDL app.

### What the Qt frontend currently does instead

`SceneTextureItem` hard-codes OpenGL throughout:
- `initGL()` calls the global `create_renderer(w, h)` factory directly (linked
  from `aorender_gl`)
- `renderGL()` calls `renderFrame()` and then manually restores
  `glBindFramebuffer(GL_FRAMEBUFFER, 0)` — a GL-specific fixup
- `updatePaintNode()` wraps the GL texture ID via `QRhiTexture::createFrom()`
  with `QRhiTexture::OpenGLTexture` — hard-coded format

`QQuickWindow::setGraphicsApi(QSGRendererInterface::OpenGL)` is called
unconditionally in `main.cpp` (guarded only by `!Q_OS_APPLE`).

### What needs to be implemented

Introduce an `IQtGPUBackend` interface (parallel to `IGPUBackend`) in
`apps/qt/render/` with the following responsibilities:

```cpp
class IQtGPUBackend {
  public:
    /// Graphics API to pass to QQuickWindow::setGraphicsApi() before QGuiApplication.
    virtual QSGRendererInterface::GraphicsApi graphicsApi() const = 0;

    /// Called on the render thread with a current context.
    /// Must create and return the IRenderer (calls glewInit / Metal setup internally).
    virtual std::unique_ptr<IRenderer> createRenderer(int w, int h) = 0;

    /// Wrap the renderer's native texture so QSGSimpleTextureNode can display it.
    /// Called on the render thread inside SceneTextureItem::updatePaintNode().
    virtual QRhiTexture* wrapTexture(QRhi* rhi, uintptr_t nativeTexId,
                                     int w, int h) = 0;

    /// Called on the render thread after renderFrame() to restore any GPU state
    /// that the renderer leaves dirty (e.g. GLRenderer leaves its FBO bound).
    virtual void afterRenderFrame(QQuickWindow* window) = 0;
};

std::unique_ptr<IQtGPUBackend> create_qt_gpu_backend();  // link-time factory
```

Concrete implementations:
- `apps/qt/render/GLQtBackend.cpp` — calls `create_renderer()`, restores
  `glBindFramebuffer(GL_FRAMEBUFFER, 0)`, wraps via
  `QRhiTexture::createFrom(QRhiTexture::OpenGLTexture, ...)`
- `apps/qt/render/MetalQtBackend.mm` — Metal equivalent for macOS (Phase 4+)

`SceneTextureItem` would hold an `IQtGPUBackend*` (provided by `RenderBridge`)
and call through the interface instead of hard-coding each GL call. `main.cpp`
calls `QQuickWindow::setGraphicsApi(backend->graphicsApi())` before
`QGuiApplication` construction.

This mirrors the SDL split exactly: the Qt scene-graph integration code
(`SceneTextureItem`) stays backend-agnostic; only the concrete backend files
contain GL or Metal headers.

**Files to create:**
- `apps/qt/render/IQtGPUBackend.h`
- `apps/qt/render/GLQtBackend.cpp`
- `apps/qt/render/MetalQtBackend.mm` (macOS only, Phase 4+)

**Files to modify:**
- `apps/qt/render/SceneTextureItem.h/.cpp` — replace direct GL/RHI calls with `IQtGPUBackend` calls
- `apps/qt/render/RenderBridge.h/.cpp` — add `setBackend(IQtGPUBackend*)` / `backend()`
- `apps/qt/main.cpp` — construct backend, pass to RenderBridge, call `backend->graphicsApi()`
- `apps/qt/CMakeLists.txt` — add `GLQtBackend.cpp` to sources; guard `MetalQtBackend.mm` on Apple

---

## How to build

```
cmake -B build -DCMAKE_BUILD_TYPE=Debug -DAO_BUILD_QT=ON
cmake --build build --target aoqt
```

Qt 6.8 required. On Windows: OpenSSL must be on `PATH` or `CMAKE_PREFIX_PATH` for HTTPS server-list fetch.

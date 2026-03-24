# Asset Pipeline Design Document

This document specifies the complete asset discovery, loading, caching, prefetching, and GPU upload pipeline. An engineer should be able to implement this system from these specifications alone.

---

## 1. Design Goals

1. **Decouple loading from rendering.** Assets are decoded on the game thread; GPU textures are created lazily on the render thread. The two threads never share mutable state directly.
2. **Automatic lifetime management via `shared_ptr` pinning.** Any code holding a `shared_ptr<Asset>` prevents cache eviction. When all holders release, the asset becomes evictable.
3. **Format-agnostic probing.** Callers request assets by path *without extension*. The library probes a prioritized list of formats and tries all decoders as a fallback for mislabeled files.
4. **Pluggable storage backends.** Directories, 7z archives, and HTTP servers are all queried through the same `Mount` interface, searched in priority order.
5. **Zero-copy GPU upload path.** Pixel data is stored in page-aligned buffers so that Metal can wrap them with `newBufferWithBytesNoCopy` on Apple Silicon, eliminating GPU-side duplication.
6. **Weak-pointer GPU texture tracking.** The renderer holds `weak_ptr<ImageAsset>` in its texture cache. When the CPU-side asset is evicted, the `weak_ptr` expires and the GPU texture is cleaned up on the next eviction sweep.

---

## 2. Architecture Overview

```
┌──────────────────────────────────────────────────────────┐
│                      MediaManager                        │
│                       (singleton)                        │
│  Owns:  MountManager  +  AssetLibrary(256 MB cache)     │
└─────────────┬────────────────────┬───────────────────────┘
              │                    │
              ▼                    ▼
┌─────────────────────┐  ┌──────────────────────────────┐
│    MountManager     │  │        AssetLibrary           │
│  (virtual filesystem│  │  (cached format-probed loader)│
│   shared_mutex)     │  │  Owns: AssetCache             │
│                     │  │                               │
│  Mount[0]: Dir      │  │  image(path) → ImageAsset     │
│  Mount[1]: Archive  │  │  audio(path) → SoundAsset     │
│  Mount[2]: HTTP     │  │  config(path) → IniDocument   │
│  Mount[3]: Embedded │  │  shader(path) → ShaderAsset   │
└─────────────────────┘  └──────────────────────────────┘
                                    │
              ┌─────────────────────┤
              ▼                     ▼
┌──────────────────────┐  ┌─────────────────────────┐
│  AOAssetLibrary      │  │     Game Thread          │
│  (AO2 path logic)    │  │  AnimationPlayer holds   │
│  Wraps AssetLibrary  │  │  shared_ptr<ImageAsset>  │
│  with fallback chains│  │          │                │
└──────────────────────┘  │          ▼                │
                          │  Layer(asset, frame, z)   │
                          │          │                │
                          │  RenderState              │
                          │          │                │
                          │  StateBuffer::present()   │
                          └──────────┬────────────────┘
                                     │ triple-buffer
                          ┌──────────▼────────────────┐
                          │     Render Thread          │
                          │  StateBuffer::update()     │
                          │          │                 │
                          │  GLRenderer::draw()        │
                          │   get_texture_array(asset) │
                          │   texture_cache[weak_ptr]  │
                          │          │                 │
                          │  Every 60 frames:          │
                          │   evict_expired_textures() │
                          └────────────────────────────┘
```

---

## 3. Data Types

### 3.1 Asset (abstract base)

```cpp
class Asset {
public:
    const std::string& path() const;     // Virtual path, no extension. Cache key.
    const std::string& format() const;   // Resolved extension: "png", "apng", "webp", etc.
    virtual size_t memory_size() const = 0; // Bytes of decoded data. Used for cache accounting.
protected:
    Asset(std::string path, std::string format);
};
```

All concrete asset types (`ImageAsset`, `SoundAsset`, `ShaderAsset`, `RawAsset`) inherit from `Asset`.

### 3.2 ImageAsset

```cpp
struct DecodedFrame {
    std::vector<uint8_t> pixels;  // RGBA, width * height * 4 bytes
    int width, height;
    int duration_ms;              // 0 = static, >0 = animation frame
};

struct ImageFrame {               // Metadata only; no pixel ownership
    int width, height;
    int duration_ms;
};

class ImageAsset : public Asset {
public:
    // Constructor packs all DecodedFrame pixel data into one contiguous
    // page-aligned AlignedBuffer. All frames must have the same dimensions.
    ImageAsset(std::string path, std::string format, std::vector<DecodedFrame> frames);

    bool is_animated() const;
    int frame_count() const;
    const ImageFrame& frame(int index) const;

    const uint8_t* frame_pixels(int index) const;  // Pointer into the contiguous buffer
    uint8_t* frame_pixels_mut(int index);           // Mutable (for glyph atlas updates)

    int width() const;   // Width of first frame (all frames uniform)
    int height() const;

    // In-place pixel update. Bumps generation counter. Handles atlas resizing.
    void update_frame(int index, const std::vector<uint8_t>& pixels);
    uint64_t generation() const;

    const AlignedBuffer& pixel_data() const;
    size_t bytes_per_frame() const;
    size_t memory_size() const override;  // Returns pixel_data_.size()
};
```

**Key decisions:**
- **Page-aligned buffer (`AlignedBuffer`)**: Allocated with `posix_memalign` / `_aligned_malloc` at the OS page size (4096 bytes). Enables Metal zero-copy texture creation.
- **Generation counter**: Incremented by `update_frame()`. The GPU renderer compares `asset->generation()` against its cached value to detect stale textures and update them in place (via `glTexSubImage3D`) rather than reallocating.
- **Uniform frame dimensions**: All frames in one asset share the same width and height. Pixels are packed sequentially: frame *i* starts at offset `i * width * height * 4`.

### 3.3 AlignedBuffer

A RAII wrapper around a page-aligned heap allocation. Supports copy, move, and range construction. Provides `data()`, `size()`, `begin()`/`end()` iterators. Reports `allocated_size()` (rounded up to page boundary) separately from logical `size()`.

---

## 4. Virtual Filesystem (MountManager)

### 4.1 Mount (abstract base)

```cpp
class Mount {
public:
    Mount(const std::filesystem::path& target_path);
    virtual void load() = 0;                          // Index contents
    virtual bool seek_file(const std::string& path) const = 0;
    virtual std::vector<uint8_t> fetch_data(const std::string& path) = 0;
protected:
    const std::filesystem::path path;
    virtual void load_cache() = 0;   // Optional disk-persisted index
    virtual void save_cache() = 0;
};
```

### 4.2 Concrete Mount Backends

| Backend | `load()` | `seek_file()` | `fetch_data()` |
|---------|----------|---------------|-----------------|
| **MountDirectory** | Validates path is a directory. No pre-indexing. | `filesystem::exists(path / relative)` | `ifstream` read into `vector<uint8_t>` |
| **MountArchive** | Opens 7z archive via bit7z. Iterates all items to build `unordered_map<string, uint32_t>` (path → archive item index). | `static_cache.contains(path)` | `reader->extractTo(buffer, index)` |
| **MountHttp** | Connects to asset server. Downloads `extensions.json` for server-advertised format preferences. | Checks internal download cache | Returns bytes from completed downloads |
| **MountEmbedded** | Scans embedded binary data (shaders, default assets) | Map lookup | Returns embedded byte range |

### 4.3 MountManager

```cpp
class MountManager {
public:
    void load_mounts(const std::vector<std::filesystem::path>& paths);  // exclusive lock
    void add_mount(std::unique_ptr<Mount> mount);                       // exclusive lock
    std::optional<std::vector<uint8_t>> fetch_data(const std::string& relative_path); // shared lock
    void prefetch(const std::string& relative_path, int priority = 1);  // shared lock
    void release_http(const std::string& relative_path);
    void release_all_http();
    std::vector<std::string> http_extensions(int asset_type) const;
private:
    mutable std::shared_mutex lock;
    std::vector<std::unique_ptr<Mount>> loaded_mounts;  // searched in order
};
```

**Search semantics:** `fetch_data()` iterates `loaded_mounts` front to back. The first mount that contains the file wins. This means:
- Local directories override archives
- Archives override HTTP
- HTTP is the fallback of last resort
- `MountEmbedded` is always last (appended by `load_mounts`)

**Thread safety:** `shared_mutex` protects the mount list. `fetch_data()` and `prefetch()` take shared locks (concurrent reads). `load_mounts()` and `add_mount()` take exclusive locks.

---

## 5. AssetCache (LRU + Pinning)

```cpp
class AssetCache {
public:
    explicit AssetCache(size_t max_bytes);

    shared_ptr<Asset> get(const string& path);   // Lookup + LRU promotion
    shared_ptr<Asset> peek(const string& path) const; // Lookup without promotion
    void insert(shared_ptr<Asset> asset);         // Insert + trigger eviction
    void evict();                                  // Manual eviction nudge

    size_t used_bytes() const;
    size_t max_bytes() const;
    size_t entry_count() const;

private:
    mutex mutex_;                                  // Guards all mutable state
    size_t max_bytes_;
    size_t used_bytes_ = 0;

    list<string> lru;                              // Front = MRU, back = LRU
    struct Entry {
        shared_ptr<Asset> asset;
        list<string>::iterator lru_pos;            // O(1) promotion via splice
    };
    unordered_map<string, Entry> entries;

    void evict_locked();                           // Eviction under held lock
};
```

### 5.1 LRU Promotion

`get()` calls `lru.splice(lru.begin(), lru, it->second.lru_pos)` to move the entry to the front of the LRU list. This is O(1) via the stored iterator. `peek()` does not promote — it's for read-only queries (e.g., config caching checks).

### 5.2 Insertion

`insert()` adds the asset at the front of the LRU list. If a prior entry exists at the same path, it's removed first and its `memory_size()` is subtracted from `used_bytes_`. After insertion, `evict_locked()` runs if the cache is over budget.

### 5.3 Eviction Algorithm

```
evict_locked():
    if used_bytes_ <= max_bytes_: return

    Walk LRU list from back (least recently used) toward front:
        For each entry:
            if entry.asset.use_count() == 1:
                // Only the cache holds a reference → safe to evict
                used_bytes_ -= entry.asset->memory_size()
                Remove from LRU list and entries map
            else:
                // Externally held (pinned) → skip
                continue

        Stop when used_bytes_ <= max_bytes_ or list exhausted
```

**Pinning contract:** Any `shared_ptr<Asset>` obtained from the cache increments `use_count` above 1. The asset cannot be evicted while any external holder exists. This is the sole mechanism for keeping assets alive — there are no manual pin/unpin calls.

---

## 6. AssetLibrary (Format-Probed Loading)

```cpp
class AssetLibrary {
public:
    AssetLibrary(MountManager& mounts, size_t cache_max_bytes);

    shared_ptr<ImageAsset> image(const string& path);
    shared_ptr<SoundAsset> audio(const string& path);
    shared_ptr<SoundAsset> audio_exact(const string& path);
    optional<IniDocument> config(const string& path);
    shared_ptr<ShaderAsset> shader(const string& path);
    optional<vector<uint8_t>> raw(const string& path);

    void prefetch(const string& path, const vector<string>& extensions, int priority = 1);
    void prefetch_image(const string& path);
    void prefetch_audio(const string& path);
    void prefetch_config(const string& path);

    void register_asset(shared_ptr<Asset> asset);
    void evict();

private:
    optional<pair<string, vector<uint8_t>>> probe(const string& path, const vector<string>& extensions);
    MountManager& mounts;
    AssetCache cache_;
};
```

### 6.1 Extension Probing (`probe()`)

```
probe(path, extensions):
    for each ext in extensions:
        candidate = path + "." + ext
        data = mounts.fetch_data(candidate)
        if data has value:
            return (candidate, data)
    return nullopt
```

Extension priority orders:
- **Images:** `webp → apng → gif → png`
- **Audio:** `opus → ogg → mp3 → wav`
- **Shaders:** `glsl/vert` or `metal` depending on backend

### 6.2 Image Loading (`image()`)

```
image(path):
    1. Check cache: cache_.get(path) → if hit, return as ImageAsset
    2. Probe: probe(path, ["webp", "apng", "gif", "png"])
       → returns (resolved_path, raw_bytes)
    3. Extract format from resolved_path (e.g. "png")
    4. Try decoders that match the format extension first
    5. If no decoder matched or matched decoder failed:
       Try ALL decoders as fallback (handles mislabeled files)
    6. If still no frames decoded: return nullptr
    7. Construct ImageAsset(path, format, decoded_frames)
    8. cache_.insert(asset) → triggers eviction if over budget
    9. Release HTTP variants: for each image extension,
       call mounts.release_http(path + "." + ext)
    10. Return shared_ptr<ImageAsset>
```

Step 9 is important: after decoding, the raw compressed bytes in the HTTP mount's download cache are released. The decoded pixel data now lives in the `AssetCache`. This prevents double-buffering of compressed + decoded data.

### 6.3 Audio Loading (`audio()` and `audio_exact()`)

Same pattern as `image()` but with audio extensions (`opus → ogg → mp3 → wav`) and audio decoders. `audio_exact()` skips probing — the caller provides the full path with extension. The cache key strips the extension so that `audio("music/track")` and `audio_exact("music/track.opus")` share a cache entry.

### 6.4 Config Loading (`config()`)

```
config(path):
    1. peek(path) in cache → if found, extract raw bytes from RawAsset
    2. Otherwise: mounts.fetch_data(path) to get raw bytes
    3. If not found: return nullopt
    4. If not already cached: cache as RawAsset, release HTTP bytes
    5. Parse INI: section headers [Name], key=value pairs
    6. Return IniDocument (map<string, map<string, string>>)
```

Note: `config()` uses `peek()` (no LRU promotion) for the cache check, since configs are accessed infrequently and shouldn't displace actively-used image assets.

---

## 7. AOAssetLibrary (AO2-Specific Path Logic)

This layer wraps `AssetLibrary` with all AO2 path conventions. Nothing outside `plugins/ao/` knows about position names, `(a)`/`(b)` prefixes, theme directories, or fallback chains.

```cpp
class AOAssetLibrary {
public:
    AOAssetLibrary(AssetLibrary& assets, const string& theme = "default");

    // Character sprites
    shared_ptr<ImageAsset> character_emote(const string& char, const string& emote, const string& prefix);
    shared_ptr<ImageAsset> character_icon(const string& char);
    shared_ptr<ImageAsset> emote_icon(const string& char, int index);
    optional<AOCharacterSheet> character_sheet(const string& char);

    // Backgrounds
    shared_ptr<ImageAsset> background(const string& name, const string& position, bool no_default = false);
    shared_ptr<ImageAsset> desk_overlay(const string& name, const string& position);

    // Theme
    shared_ptr<ImageAsset> theme_image(const string& element);
    optional<IniDocument> theme_config(const string& filename);

    // Prefetch
    void prefetch_character(const string& char, const string& emote, const string& pre_emote, int priority);
    void prefetch_own_character(const string& char);
    void prefetch_background(const string& name, const string& position, int priority);
    void prefetch_theme();

    // Audio
    shared_ptr<SoundAsset> sound_effect(const string& sfx_name);
    shared_ptr<SoundAsset> blip_sound(const string& blip_name);
};
```

### 7.1 Character Emote Fallback Chain

```
character_emote(character, emote, prefix):
    base = "characters/{character}/"

    1. Try: assets.image(base + prefix + emote)
       e.g. "characters/Phoenix/(a)normal"
    2. If found and prefix was non-empty:
       Release bare-name HTTP variants (they were prefetched but not needed)
    3. If not found and prefix was non-empty:
       Try: assets.image(base + emote)
       e.g. "characters/Phoenix/normal"
    4. If still not found:
       Re-prefetch with default extensions (server-advertised extensions may
       have missed a format). The emote player's retry loop picks this up later.
    5. Return result (may be nullptr)
```

### 7.2 Background Fallback Chain

```
background(name, position, no_default):
    legacy = bg_filename(position)  // "def" → "defenseempty", "pro" → "prosecutorempty", etc.

    1. Try: assets.image("background/{name}/{legacy}")
    2. If not found and legacy != position:
       Try: assets.image("background/{name}/{position}")  // modern naming
    3. If not found and !no_default and name != "default":
       Try: assets.image("background/default/{legacy}")   // default bg fallback
    4. Return result
```

### 7.3 Theme Fallback Chain

```
theme_image(element):
    1. Try: assets.image("themes/{active_theme}/{element}")
    2. If not found and active_theme != "default":
       Try: assets.image("themes/default/{element}")
    3. Return result
```

### 7.4 Font Search Order

```
find_font(normalized_name):
    candidates = [
        "{name}.ttf", "{name_underscored}.ttf",
        "{name}-regular.ttf", "{name_underscored}_regular.ttf",
        "{name}_Regular.ttf", "{name_underscored}_Regular.ttf"
    ]
    directories = [
        "themes/{active_theme}/",
        "themes/default/",
        "themes/AceAttorney DS/",
        "themes/AceAttorney2x/",
        "themes/AceAttorney 2x/",
        "fonts/"
    ]
    for each dir in directories:
        for each candidate in candidates:
            result = assets.raw(dir + candidate)
            if result: return result
    return nullopt
```

### 7.5 Position Name Mappings

| Short code | Background filename | Desk filename |
|------------|-------------------|---------------|
| `def` | `defenseempty` | `defensedesk` |
| `pro` | `prosecutorempty` | `prosecutiondesk` |
| `wit` | `witnessempty` | `stand` |
| `jud` | `judgestand` | `judgedesk` |
| `hld` | `helperstand` | `helperdesk` |
| `hlp` | `prohelperstand` | `prohelperdesk` |
| `jur` | `jurystand` | `jurydesk` |
| `sea` | `seancestand` | `seancedesk` |
| *(other)* | *(passthrough)* | `{pos}_overlay` |

---

## 8. Prefetching

Prefetching triggers background HTTP downloads for assets not yet available in local mounts. Downloads are non-blocking; the data becomes available on a future `fetch_data()` call.

### 8.1 AssetLibrary Prefetch

```
prefetch(path, extensions, priority):
    for each ext in extensions:
        mounts.prefetch(path + "." + ext, priority)

prefetch_image(path):
    if cache already contains path: return (skip)
    prefetch(path, ["webp", "apng", "gif", "png"])
```

### 8.2 MountManager Prefetch

```
prefetch(relative_path, priority):
    shared_lock
    if any LOCAL (non-HTTP) mount has the file: return (skip)
    for each HTTP mount:
        mount.request(relative_path, priority)
```

### 8.3 AOAssetLibrary Prefetch Strategy

**On incoming IC message (other player's character):** `prefetch_character()` at normal priority.
```
prefetch_character(character, emote, pre_emote, priority):
    base = "characters/{character}/"
    prefetch: base + "(a)" + emote    // talking
    prefetch: base + "(b)" + emote    // idle
    prefetch: base + emote            // bare fallback
    prefetch: base + pre_emote        // pre-animation (if not "-")
    prefetch: base + "char.ini"       // character config
    prefetch: blip sounds (if char.ini already cached)
```

**On player selecting own character:** `prefetch_own_character()` at HIGH priority (2).
```
prefetch_own_character(character):
    Load char.ini → get emote list
    For each emote:
        prefetch: emote button icon
        prefetch: (a){emote}, (b){emote}, bare {emote}
        prefetch: pre-animation (if any)
    Eagerly decode blip sound into cache
```

**On entering courtroom:** `prefetch_theme()` for chatbox images.

**On background change:** `prefetch_background()` for legacy + modern names + desk overlay.

### 8.4 Server-Advertised Extensions

The HTTP mount downloads `extensions.json` from the asset server, which advertises preferred formats per asset type (e.g., the server might say characters are available in `webp` only). `AOAssetLibrary::prefetch_image()` checks `mounts.http_extensions(asset_type)` and uses those if available, falling back to the default extension list.

### 8.5 HTTP Cache Cleanup

After an asset is decoded and inserted into the `AssetCache`, the raw compressed bytes sitting in the HTTP mount's download cache are released:

```cpp
// In AssetLibrary::image(), after successful decode:
for (const auto& ext : supported_image_extensions())
    mounts.release_http(path + "." + ext);
```

Additionally, the `AOCourtroomPresenter` calls `release_all_http()` every ~30 seconds to sweep any orphaned HTTP cache entries.

---

## 9. Rendering Pipeline

### 9.1 AnimationPlayer

A pure-logic frame sequencer. Holds a `shared_ptr<ImageAsset>` (pinning it in cache) and advances through frames based on elapsed time.

```cpp
class AnimationPlayer {
public:
    void load(shared_ptr<ImageAsset> asset, bool loop);
    void clear();               // Releases the asset (unpins from cache)
    void tick(int delta_ms);    // Advance by delta; updates frame_index
    bool finished() const;      // True when one-shot is done
    bool has_frame() const;
    int current_frame_index() const;
    const ImageFrame* current_frame() const;
    const shared_ptr<ImageAsset>& asset() const;
};
```

**Tick logic:**
```
tick(delta_ms):
    if no asset or done: return
    elapsed_ms += delta_ms
    while elapsed_ms >= current_frame.duration_ms:
        elapsed_ms -= current_frame.duration_ms
        frame_index++
        if frame_index >= frame_count:
            if looping: frame_index = 0
            else: frame_index = frame_count - 1; done = true; break
```

### 9.2 Layer and LayerGroup

```cpp
class Layer {
    shared_ptr<ImageAsset> asset;  // Holds ref → pins in cache
    int frame_index;               // Which animation frame to display
    uint16_t z_index;              // Draw order within group
    float opacity = 1.0f;
    shared_ptr<ShaderAsset> shader_;
    shared_ptr<MeshAsset> mesh_;
    Transform transform_;
};

class LayerGroup {
    map<int, Layer> layers;        // Layers keyed by ID, iterated in map order
    shared_ptr<ShaderAsset> shader_;
    Transform transform_;
};
```

### 9.3 RenderState

```cpp
class RenderState {
    map<int, LayerGroup> layer_groups;  // Groups keyed by ID, drawn in map order
};
```

The game thread builds a `RenderState` each tick by assembling `LayerGroups` containing `Layers`. Each `Layer` carries a `shared_ptr<ImageAsset>` and a `frame_index`.

### 9.4 StateBuffer (Triple-Buffering)

Three `RenderState` slots enable lock-free communication between the game thread and render thread:

```
Slot         | Owner          | Purpose
-------------|----------------|----------------------------------------
preparing    | game thread    | Currently being written by game logic
ready        | shared         | Most recently published state
presenting   | render thread  | Currently being read for drawing
```

**Game thread** calls:
1. `get_producer_buf()` → returns `preparing`
2. Write layers into the RenderState
3. `present()` → under mutex, swap `preparing` ↔ `ready`, set `stale = true`

**Render thread** calls:
1. `update()` → if `stale`, under mutex swap `ready` ↔ `presenting`, clear `stale`
2. `get_consumer_buf()` → returns `presenting`
3. Draw the RenderState

**Properties:**
- Game thread never blocks on the render thread
- At most one frame of latency between publish and display
- The mutex only guards pointer swaps (nanoseconds), not data copies

### 9.5 GLRenderer Texture Cache

The GL renderer maintains a separate texture cache keyed by raw `ImageAsset*` pointer:

```cpp
struct TextureCacheEntry {
    weak_ptr<ImageAsset> asset;  // Weak reference for expiry detection
    GLuint texture;              // GL_TEXTURE_2D_ARRAY handle
    uint64_t generation;         // Last-seen asset generation
};
unordered_map<const ImageAsset*, TextureCacheEntry> texture_cache;
```

#### Texture Upload (`get_texture_array`)

```
get_texture_array(asset):
    1. Look up texture_cache[asset.get()]
    2. If found and generation matches: return cached texture (fast path)
    3. If found but generation stale:
       - glBindTexture(GL_TEXTURE_2D_ARRAY, cached_texture)
       - For each frame: glTexSubImage3D to update pixels in place
       - Update stored generation
       - Return texture
    4. If not found (first encounter):
       - glGenTextures → new texture
       - glTexImage3D(GL_TEXTURE_2D_ARRAY, RGBA8, width, height, frame_count)
       - For each frame: glTexSubImage3D with frame_pixels(i)
       - Set filtering: GL_NEAREST (pixel-perfect)
       - Set wrapping: GL_CLAMP_TO_EDGE
       - Store TextureCacheEntry with weak_ptr<ImageAsset>
       - Return texture
```

All frames of an animated asset are packed into a single `GL_TEXTURE_2D_ARRAY`. The fragment shader selects the active frame via `sampler2DArray` + `uniform int frame_index`. This avoids per-frame texture binding.

#### Texture Eviction

Every 60 draw calls:
```
evict_expired_textures():
    for each entry in texture_cache:
        if entry.asset.expired():    // weak_ptr no longer valid
            glDeleteTextures(entry.texture)
            remove from cache
    // Same for preview_cache_ (GL_TEXTURE_2D, frame 0 only, for ImGui)
```

#### Preview Cache (Separate)

ImGui expects `GL_TEXTURE_2D` for its `Image()` widget. A separate `preview_cache_` stores single-frame (frame 0) `GL_TEXTURE_2D` textures with `GL_LINEAR` filtering for UI thumbnails. This prevents allocating full texture arrays for assets that only need a preview.

---

## 10. Complete Asset Lifecycle

```
                     ┌─────────────────────┐
                     │    Asset Request     │
                     │  (e.g. image(path))  │
                     └─────────┬───────────┘
                               │
                    ┌──────────▼──────────┐
                    │  cache_.get(path)?   │──── hit ───► return shared_ptr
                    └──────────┬──────────┘              (promotes LRU)
                               │ miss
                    ┌──────────▼──────────┐
                    │  probe(path, exts)  │
                    │  webp→apng→gif→png  │
                    │  via MountManager   │
                    └──────────┬──────────┘
                               │ found
                    ┌──────────▼──────────┐
                    │  Decode raw bytes   │
                    │  → DecodedFrame[]   │
                    └──────────┬──────────┘
                               │
                    ┌──────────▼──────────┐
                    │  Construct asset    │
                    │  Pack into          │
                    │  AlignedBuffer      │
                    └──────────┬──────────┘
                               │
                    ┌──────────▼──────────┐
                    │  cache_.insert()    │──► evict LRU if over 256 MB
                    └──────────┬──────────┘
                               │
                    ┌──────────▼──────────┐
                    │  release_http()     │   Free raw download bytes
                    └──────────┬──────────┘
                               │
                    ┌──────────▼──────────┐
                    │  Return shared_ptr  │   Caller now pins asset
                    └──────────┬──────────┘
                               │
              ┌────────────────┼────────────────┐
              ▼                ▼                 ▼
      AnimationPlayer     Layer in          Other holder
      holds shared_ptr    RenderState       (e.g. UI)
              │                │
              ▼                ▼
      Asset pinned      Asset pinned
      (use_count > 1)   (use_count > 1)
              │                │
              │    ┌───────────▼───────────┐
              │    │  StateBuffer.present() │
              │    └───────────┬───────────┘
              │                │
              │    ┌───────────▼───────────┐
              │    │  Render thread reads   │
              │    │  get_texture_array()   │
              │    │  → lazy GPU upload     │
              │    │  texture_cache stores  │
              │    │  weak_ptr<ImageAsset>  │
              │    └───────────────────────┘
              │
              ▼ (caller releases shared_ptr)
      use_count drops to 1 (cache only)
              │
              ▼ (next evict() pass, if over budget)
      cache removes entry
      shared_ptr destroyed → weak_ptr expires
              │
              ▼ (next 60-frame eviction sweep)
      GLRenderer::evict_expired_textures()
      glDeleteTextures → GPU memory freed
```

---

## 11. Thread Safety Summary

| Component | Lock Type | Scope |
|-----------|-----------|-------|
| **AssetCache** | `std::mutex` | All of `get`, `peek`, `insert`, `evict` |
| **MountManager** | `std::shared_mutex` | Shared for `fetch_data`/`prefetch`; exclusive for `load_mounts`/`add_mount` |
| **StateBuffer** | `std::mutex` + `atomic<bool>` | Mutex guards pointer swaps only; atomic for stale flag |
| **AnimationPlayer** | None | Game thread only |
| **GLRenderer** | None | Render thread only |
| **EventChannel** | Per-channel `std::mutex` | Thread-safe publish/consume |

The three threads (network, game, render) communicate exclusively through `EventChannel` queues and the `StateBuffer`. No shared mutable state exists between them outside of these two mechanisms.

---

## 12. Periodic Maintenance

The `AOCourtroomPresenter` runs two maintenance tasks every ~30 seconds from the game thread:

```cpp
evict_timer_ms += delta_ms;
if (evict_timer_ms >= 30000) {
    evict_timer_ms = 0;
    ao_assets->engine_assets().evict();              // CPU cache eviction
    MediaManager::instance().mounts_ref().release_all_http(); // HTTP cache cleanup
}
```

The GL renderer runs its own cleanup every 60 frames (approximately once per second at 60fps):
```cpp
if (++frame_counter % 60 == 0) {
    evict_expired_textures();
    evict_expired_meshes();
}
```

---

## 13. MediaManager (System Entry Point)

```cpp
class MediaManager {  // Singleton
public:
    static MediaManager& instance();
    void init(const std::filesystem::path& base_path);  // Can be called multiple times
    AssetLibrary& assets();
    MountManager& mounts_ref();
    void shutdown();  // Call before main() returns
private:
    unique_ptr<MountManager> mounts;
    unique_ptr<AssetLibrary> library;  // 256 MB cache budget
};
```

`init()` creates the `MountManager`, calls `load_mounts({base_path})`, then creates an `AssetLibrary` with a 256 MB cache budget. `shutdown()` tears down both in the correct order before static destruction (necessary because bit7z internals and GL contexts have static-lifetime dependencies).

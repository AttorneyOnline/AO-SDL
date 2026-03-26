# Protocol v2 Proposal

This document proposes a replacement for the AO2 client-server protocol, server architecture, and asset system. It is a clean-break redesign — not an extension of the existing `#`-delimited packet format.

---

## 1. Design Pillars

### 1.1 Structured, Versioned Wire Format

The primary wire format is **JSON with an explicit schema version** in every message. Messages are self-describing: no positional field indices, no delimiter escaping, no heuristic parsing. Every message carries a version tag so that both endpoints can interpret it unambiguously.

Rationale: The AO2 protocol grew organically by appending positional fields to packets like `MS` (now 32+ fields). A missing or extra field silently shifts every subsequent value. JSON with named keys eliminates this entire class of bug, and a version tag ensures that both sides agree on what the keys mean.

### 1.2 REST-Primary, WebSocket-Broadcast Architecture

The protocol is split across two transports with a clear division of responsibility:

- **HTTP REST API** — All client-initiated actions and all server-side state. Authentication, session management, character selection, asset queries, evidence manipulation, room joins, IC/OOC message submission — anything the client *does* is an HTTP request. All server-side state is directly queryable via REST endpoints scoped to the client's session token.
- **WebSocket** — One-way server-to-client push of **ephemeral broadcast data**. IC messages, OOC chat, background changes, music cues, timer updates, presence changes — anything the server broadcasts to the room. The WebSocket carries no client-to-server application traffic.

Message history (IC, OOC) is ephemeral by design. It is broadcast, not stored as queryable state. A client that misses messages while disconnected does not retrieve them — it rejoins the live stream. Nearly everything else (room state, character assignments, evidence lists, health bars, player metadata) is durable server-side state accessible through the REST API at any time.

Rationale: This split makes the protocol trivially testable — every state mutation is a request/response pair with a well-defined result. The WebSocket becomes a pure notification channel with no request/response semantics to manage, no ordering dependencies, and no failure modes beyond "reconnect and re-subscribe." Clients can always recover full state from REST after a reconnect; the WebSocket just keeps them up to date in real time.

### 1.3 Session Tokens, Not Sockets

A session is scoped to a **server-managed token**, not to a TCP connection. Dropping a WebSocket does not destroy the session. The client reconnects, presents its token, and resumes. The server is the sole authority on session lifetime — it issues tokens, validates them, and expires them.

Rationale: The AO2 protocol ties identity to a live socket. A brief network interruption forces a full rejoin: re-receive character list, re-select character, lose OOC context. Token-scoped sessions decouple logical presence from transport health.

### 1.4 Federated Authentication

Authentication is backed by an **external identity provider** — not by the server operator. The server verifies identity tokens issued by a third-party IdP (OAuth 2.0 / OpenID Connect). Server operators never handle user credentials directly.

The system should be as decentralized as practical: no single identity provider should be a hard dependency. Servers may accept tokens from multiple IdPs, and the protocol should accommodate this without special-casing any provider.

### 1.5 Full Forward Compatibility

Every behavior expressible in the AO2 protocol must be expressible in v2. This is a superset, not a subset. Features that were bolted onto AO2 (frame-triggered effects, per-animation SFX, area updates, timers, evidence, player metadata) become first-class structured messages rather than overloaded field indices.

Additionally, **all versions of the v2 protocol must remain forward-compatible with each other.** A v2.0 client connecting to a v2.3 server must not crash or enter an undefined state. Unrecognized message types and unknown fields are ignored gracefully, never fatally. The WebSocket subprotocol header is leveraged for version negotiation at connection time, allowing dynamic selection between protocol versions — including fallback to the legacy AO2 protocol during a transition period.

### 1.6 Content-Addressable Assets

Assets are identified by the **hash of their contents**, not by convention-based filesystem paths. A character sprite is not `characters/Phoenix/normal.webp` — it is a blob with a content address, referenced by a manifest that gives it semantic meaning.

Rationale: Convention-based paths create tight coupling between directory layout, filename casing, and client logic. Content addressing makes caching trivial (same hash = same bytes, forever), deduplication automatic, and CDN delivery straightforward.

### 1.7 Bundle-Oriented Asset Delivery

An entire asset bundle (e.g. a full character, a complete background set) must be retrievable in **a single HTTP transaction**. Clients should not need to discover and fetch dozens of individual files through iterative probing.

Large audio assets (music tracks, ambient loops) are the exception: these should support **chunked streaming delivery** so that playback can begin before the full file is downloaded.

### 1.8 Tool-Oriented Asset Authorship

The asset format should be designed for **tooling, not hand-editing**. AO2 assets are authored by manually naming files according to undocumented conventions and editing `char.ini` in a text editor. Errors are discovered only at runtime — or not at all.

The v2 asset format should be structured (JSON manifests, not INI), validatable (schema-checkable before upload), and designed with the assumption that dedicated authoring tools will produce and consume it. Human readability is welcome but is not the primary design constraint.

### 1.9 First-Class Asset Protocol Integration

Asset query and delivery are **part of the protocol**, not a side channel. The REST API provides structured endpoints for discovering available assets, resolving content addresses, downloading bundles, and streaming audio. The server can advertise what assets it hosts, what formats it prefers, and what a client needs to prefetch — all through well-defined API contracts rather than ad-hoc URL conventions.

---

## 2. Non-Goals

- **Backward wire compatibility with AO2.** The v2 protocol is not an extension of the `#%`-delimited format. Interoperability with AO2 servers/clients is handled at the transport negotiation layer (WebSocket subprotocol headers), not by encoding v2 messages in AO2 framing.
- **Mandating a specific identity provider.** The protocol defines how identity tokens are presented and verified, not who issues them.
- **Prescribing server implementation language or stack.** The protocol is a contract between endpoints. Server internals are out of scope.
- **Server-to-server federation.** Out of scope for this proposal.

---

## 3. Schema Versioning

Schemas are hosted as the **source of truth** on the official GitHub project. Each schema document is versioned independently — a change to the IC message schema increments that schema's version without affecting unrelated schemas. Versions are **simple monotonic integers starting at 1**. There is no semver; a version number is a pointer to an exact document revision.

This means a server and client do not need to agree on a single global protocol version. They agree per-schema: "I understand `ic_message` at version 3 and `evidence` at version 1." Forward compatibility (section 1.5) ensures that a newer schema version can always be consumed by an older client — unknown fields are ignored, never fatal.

---

## 4. Transport Negotiation

WebSocket subprotocol negotiation is **string-based with sub-versioning**. Its primary role is distinguishing the legacy AO2 protocol from the v2 protocol family at connection time. Within the v2 protocol, interoperability between versions is handled at the schema level (section 3), not at the transport level.

A client advertises supported subprotocols in the WebSocket `Sec-WebSocket-Protocol` header. The server selects the highest mutually supported option. If only the AO2 subprotocol is available, the connection falls back to legacy framing. This allows a single server endpoint to serve both AO2 and v2 clients during the transition period.

---

## 5. Asset Format

### 5.1 Content Addressing

Assets are addressed by their **SHA-256 hash**. The hash is computed over the raw file bytes. Two identical files always resolve to the same address regardless of where they are hosted or what they are named.

### 5.2 Bundles

An asset bundle is a **ZIP archive** containing the asset files and a JSON manifest. The manifest lists every file in the bundle along with its content hash, semantic role (e.g. "idle animation", "talking variant", "sound effect"), and any metadata that was previously encoded in convention-based filenames or INI files.

The bundle itself is also content-addressable — its SHA-256 hash serves as the bundle address. Bundles are retrieved in a single HTTP GET request.

### 5.3 Audio Streaming

Large audio assets (music tracks, ambient loops) support delivery via **HTTP chunked transfer encoding**. This allows playback to begin before the full file is downloaded. The REST API exposes a streaming endpoint per audio asset; clients that prefer to download the full file upfront may do so via a standard non-chunked GET instead.

### 5.4 Example: Character Bundle

A character is the most complex asset type in Attorney Online. In AO2, a character is defined by a directory of convention-named files (`(a)normal.webp`, `(b)normal.webp`, `emotions/button1_off.png`, ...) plus a `char.ini` file with ~9 INI sections encoding emotes, sound effects, frame-triggered effects, and display metadata. Authoring errors — misspelled filenames, wrong section keys, off-by-one emote indices — are only discovered at runtime.

In v2, a character bundle is a ZIP archive containing a `manifest.json` and the asset files it references. The manifest makes every relationship explicit: which files are the idle/talking/pre-animation variants of which emote, what SFX plays at what frame, what the display metadata is. Nothing is inferred from filenames or directory structure.

#### Manifest structure

```json
{
  "schema": "character",
  "schema_version": 1,
  "name": "Phoenix Wright",
  "icon": "icon.webp",
  "defaults": {
    "side": "def",
    "blip": "male"
  },
  "emotes": [
    {
      "name": "Normal",
      "idle": "normal-idle.webp",
      "talking": "normal-talk.webp",
      "button": "normal-button.webp",
      "desk": "show",
      "sfx": {
        "file": "sfx/benchslam.opus",
        "delay_ms": 0,
        "loop": false
      },
      "pre_animation": {
        "file": "normal-pre.webp",
        "duration_ms": 760
      },
      "frame_effects": [
        { "frame": 3, "type": "sfx", "file": "sfx/deskslam.opus" },
        { "frame": 5, "type": "screenshake" },
        { "frame": 12, "type": "realization" }
      ]
    },
    {
      "name": "Confident",
      "idle": "confident-idle.webp",
      "talking": "confident-talk.webp",
      "button": "confident-button.webp",
      "desk": "show"
    },
    {
      "name": "Thinking",
      "idle": "thinking-idle.webp",
      "talking": "thinking-talk.webp",
      "button": "thinking-button.webp",
      "desk": "hide"
    }
  ],
  "files": {
    "icon.webp":              "sha256:a1b2c3d4...",
    "normal-idle.webp":       "sha256:e5f6a7b8...",
    "normal-talk.webp":       "sha256:c9d0e1f2...",
    "normal-pre.webp":        "sha256:13243546...",
    "normal-button.webp":     "sha256:57687980...",
    "confident-idle.webp":    "sha256:a0b1c2d3...",
    "confident-talk.webp":    "sha256:e4f5a6b7...",
    "confident-button.webp":  "sha256:c8d9e0f1...",
    "thinking-idle.webp":     "sha256:23344556...",
    "thinking-talk.webp":     "sha256:67788990...",
    "thinking-button.webp":   "sha256:a1b2c3d4...",
    "sfx/benchslam.opus":     "sha256:d5e6f7a8...",
    "sfx/deskslam.opus":      "sha256:b9c0d1e2..."
  }
}
```

#### What the manifest replaces

| AO2 Convention | v2 Manifest Equivalent |
|---|---|
| `char.ini [Options] showname` | `name` (top-level) |
| `char.ini [Options] side` | `defaults.side` |
| `char.ini [Options] blips` | `defaults.blip` |
| `char.ini [Emotions] 1 = Comment#PreAnim#Anim#Mod#DeskMod` | `emotes[0]` object with named fields |
| `(a)emotename.webp` filename convention | `emotes[n].talking` explicit reference |
| `(b)emotename.webp` filename convention | `emotes[n].idle` explicit reference |
| `emotions/button1_off.png` naming convention | `emotes[n].button` explicit reference |
| `char.ini [SoundN]`, `[SoundT]`, `[SoundL]` sections | `emotes[n].sfx` object |
| `char.ini [Time] PreAnimName = 760` | `emotes[n].pre_animation.duration_ms` |
| `char.ini [AnimName_FrameSFX]` sections | `emotes[n].frame_effects` array |
| `char.ini [AnimName_FrameScreenshake]` sections | `emotes[n].frame_effects` array |
| `char.ini [AnimName_FrameRealization]` sections | `emotes[n].frame_effects` array |
| Extension probing (try .webp, .apng, .gif, .png) | Exact filename in manifest; format is known |
| `char_icon.png` naming convention | `icon` explicit reference |

#### What the `files` map enables

The `files` map at the bottom of the manifest lists every file in the ZIP along with its SHA-256 hash. This serves three purposes:

1. **Integrity.** The client can verify every file against its hash after extraction.
2. **Deduplication.** If two bundles reference the same SFX or sprite, the client cache recognizes the identical hash and stores only one copy.
3. **Partial fetching.** If a client already has some files cached (by content hash from another bundle), it can request only the missing files rather than re-downloading the entire ZIP. The REST API can support this via a separate endpoint that serves individual blobs by hash.

#### Background bundle (sketch)

The same pattern applies. A background in AO2 is a directory of convention-named position images (`defenseempty.png`, `prosecutiondesk.png`, ...) plus a `design.ini` with QSettings-format position origins and slide durations. In v2:

```json
{
  "schema": "background",
  "schema_version": 1,
  "name": "Courtroom",
  "positions": [
    {
      "id": "def",
      "label": "Defense",
      "image": "def.webp",
      "desk": "def-desk.webp",
      "origin_x": 128
    },
    {
      "id": "pro",
      "label": "Prosecution",
      "image": "pro.webp",
      "desk": "pro-desk.webp",
      "origin_x": 1168
    },
    {
      "id": "wit",
      "label": "Witness",
      "image": "wit.webp",
      "desk": "wit-desk.webp",
      "origin_x": 648
    }
  ],
  "transitions": [
    { "from": "def", "to": "wit", "duration_ms": 500 },
    { "from": "def", "to": "pro", "duration_ms": 550 },
    { "from": "wit", "to": "pro", "duration_ms": 500 }
  ],
  "files": {
    "def.webp":      "sha256:...",
    "def-desk.webp": "sha256:...",
    "pro.webp":      "sha256:...",
    "pro-desk.webp": "sha256:...",
    "wit.webp":      "sha256:...",
    "wit-desk.webp": "sha256:..."
  }
}
```

This replaces the legacy position name mappings (`defenseempty` vs `def`, `prosecutorempty` vs `pro`, `stand` vs `wit_overlay`), the QSettings `design.ini` format, and the fallback chain that searches both legacy and modern filenames.

#### Design notes

**The manifest is the schema, not the filesystem.** AO2 derives meaning from where a file is and what it's called. v2 derives meaning from what the manifest says about it. A file named `xyz.webp` can be an idle sprite, a talking sprite, or a background — the manifest assigns the role. This eliminates the entire class of "file named wrong" bugs.

**Emote indices are gone.** AO2 uses 1-indexed integers everywhere — `[SoundN] 3 = slam` means "the third emote plays slam.opus." Off-by-one errors are endemic. In v2, emotes are objects in an array. SFX, frame effects, and pre-animations are nested inside the emote they belong to. There is no indirection through numeric indices.

**Frame effects are unified.** AO2 uses three separate INI sections (`_FrameSFX`, `_FrameScreenshake`, `_FrameRealization`) with different value formats to express "something happens at frame N." v2 uses one `frame_effects` array with a `type` discriminator. Adding a new frame effect type is adding a new string to `type`, not adding a new INI section convention.

**Extension probing is eliminated.** AO2 clients request assets without extensions and try `.webp`, `.apng`, `.gif`, `.png` in order. The manifest lists exact filenames — the format is known at load time, no probing needed.

---

## 6. Moderation

Moderation actions are **first-class protocol members**, not out-of-band server commands. Kick, ban, and mute are all expressed as REST API calls authenticated via the caller's session token. The server validates moderator authority through the identity provider — federated authentication (section 1.4) makes this trivial since moderator roles are bound to verified identities rather than IP addresses or hardware IDs.

All moderation actions share a **single message type** with an extensible action field. Standard actions (kick, ban, mute) have well-defined schemas. Non-standard actions (disemvowel, text replacement, or other server-specific behaviors) use extension fields on the same message type, allowing servers to implement custom moderation without forking the protocol.

Rate limiting is a **server-level and HTTP-gateway-level concern**. The protocol does not prescribe rate limit values — server administrators configure these based on their infrastructure and community needs. The REST API surfaces rate limit status via standard HTTP headers (`Retry-After`, `X-RateLimit-*`) so clients can back off gracefully.

---

## 7. Migration Strategy

The transition from AO2 to v2 is designed to be **gradual and non-breaking**:

- **Client:** Retains decoupled AO2 protocol and asset backends (`net/ao`, `AOAssetLibrary`) alongside the new v2 implementations. The client can connect to both AO2 and v2 servers. All new features are implemented exclusively in the v2 protocol path.
- **Server:** The new server implements the AO2 protocol on a **best-effort basis** via decoupled network backends (mirroring the client architecture). Decisions to deprecate or drop AO2 support will be driven by community adoption and input, not by a predetermined timeline.
- **Assets:** Authoring tools will include a **conversion pipeline** for migrating AO2 assets (convention-based directories, `char.ini` files) into v2 bundles (ZIP archives with JSON manifests). This is a one-way transformation — v2 is the target format going forward.
- **HTTP Namespacing:** The entire v2 REST API and all asset delivery endpoints are **URL-namespaced** to avoid conflicts with AO2/webAO-style asset delivery over HTTP. The server can simultaneously serve legacy static file requests and v2 API requests on the same host.

---

## 8. Example Schema: Moderation Action

This section demonstrates how a single v2 schema replaces functionality currently scattered across AO2 wire packets (`KK`, `BN` when moderator-initiated), server-parsed OOC chat commands (`/ban`, `/kick`, `/mute`, `/disemvowel`, `/area_lock`, etc.), and bespoke authentication flows (`/login` with shared modpass).

The core abstraction: a moderation action is **a typed operation, optionally targeting a user, optionally scoped to an area, with an open-ended parameter bag**. The schema defines the envelope — not the vocabulary of actions. What actions exist, what parameters they accept, and what permissions they require are server concerns.

- **REST endpoint** (`POST /v2/moderation/actions`) — Submit an action, authenticated via session token.
- **WebSocket broadcast** (`moderation_action`) — Server pushes a notification to affected clients when an action takes effect.
- **REST query** (`GET /v2/moderation/actions`) — Retrieve active moderation state.

### 8.1 REST Request Schema

`POST /v2/moderation/actions`

```json
{
  "$schema": "https://json-schema.org/draft/2020-12/schema",
  "title": "moderation_action_request",
  "type": "object",
  "required": ["schema_version", "action"],
  "additionalProperties": true,
  "properties": {
    "schema_version": {
      "type": "integer",
      "const": 1
    },
    "action": {
      "type": "string",
      "description": "Server-defined action identifier (e.g. 'kick', 'ban', 'mute'). Not enumerated by the schema — servers advertise supported actions via the REST API."
    },
    "target": {
      "type": "string",
      "description": "Session ID of the affected user. Omitted for untargeted actions."
    },
    "area": {
      "type": "string",
      "description": "Area identifier. Defaults to the moderator's current area if omitted."
    },
    "reason": {
      "type": "string",
      "description": "Human-readable reason. Delivered to the target where applicable."
    },
    "duration": {
      "type": ["integer", "null"],
      "description": "Duration in seconds. null = permanent. Interpretation is action-dependent."
    },
    "params": {
      "type": "object",
      "description": "Action-specific parameters. The schema does not constrain this — the server defines what each action accepts."
    }
  }
}
```

### 8.2 WebSocket Broadcast Schema

Pushed to affected clients when an action takes effect. Carries only what clients need to react — no internal server state (IPs, hardware IDs) on the wire.

```json
{
  "$schema": "https://json-schema.org/draft/2020-12/schema",
  "title": "moderation_action_broadcast",
  "type": "object",
  "required": ["schema_version", "action", "timestamp"],
  "additionalProperties": true,
  "properties": {
    "schema_version": {
      "type": "integer",
      "const": 1
    },
    "action": {
      "type": "string"
    },
    "timestamp": {
      "type": "string",
      "format": "date-time"
    },
    "moderator": {
      "type": "string",
      "description": "Display name of the moderator. Omitted if the server hides moderator identity."
    },
    "target": {
      "type": "string",
      "description": "Display name of the affected user."
    },
    "area": {
      "type": "string"
    },
    "reason": {
      "type": "string"
    },
    "duration": {
      "type": ["integer", "null"]
    },
    "params": {
      "type": "object",
      "description": "Action-specific data for client presentation."
    }
  }
}
```

### 8.3 Usage Examples

A few concrete examples showing how existing AO2 moderation behavior maps onto this envelope:

```json
// Kick a user
{ "schema_version": 1, "action": "kick", "target": "sess_abc123", "reason": "Disruptive behavior" }

// Temporary ban (1 hour)
{ "schema_version": 1, "action": "ban", "target": "sess_abc123", "duration": 3600, "reason": "Spam" }

// Permanent ban
{ "schema_version": 1, "action": "ban", "target": "sess_abc123", "duration": null, "reason": "Repeated violations" }

// Mute a user's IC chat
{ "schema_version": 1, "action": "mute", "target": "sess_abc123", "params": { "channel": "ic" } }

// Text transformation (replaces AO2's /disemvowel, /gimp, /shake, /medieval)
{ "schema_version": 1, "action": "text_transform", "target": "sess_abc123", "params": { "transform": "disemvowel" } }

// Lock the current area
{ "schema_version": 1, "action": "lock_area", "area": "area_courtroom1" }

// Server-wide notice
{ "schema_version": 1, "action": "notice", "params": { "message": "Server restarting in 5 minutes", "scope": "server" } }

// Server-specific custom action — schema doesn't need to know about it
{ "schema_version": 1, "action": "x-my-server-custom-action", "target": "sess_abc123", "params": { "whatever": "the server expects" } }
```

### 8.4 Design Notes

**The schema is an envelope, not a dictionary.** The `action` field is a free string, not an enum. The `params` object is unconstrained. This means the protocol schema never needs to change when a server adds a new moderation capability — it just advertises a new action string. Clients that don't recognize an action can present a generic "moderation action occurred" message. The server's REST API can expose a discovery endpoint listing supported actions and their expected parameters, but that catalog is server content, not protocol structure.

**Why `target` is a session ID, not an IP or hardware ID.** The v2 protocol identifies users by session, backed by IdP-verified identities. The server may track IPs and HWIDs internally, but these are not wire protocol concepts. A moderator targets a session; the server decides what that means for multi-client enforcement.

**What this replaces in AO2.** The `KK` wire packet, `BN` when used as a moderator override, the entire `/command`-in-OOC-chat moderation system (kick, ban, mute, disemvowel, gimp, shake, medieval, area_lock, bglock, block_dj, block_wtce, charcurse, notice, invite — approximately 40 commands), and the `/login` authentication flow (eliminated entirely — moderator identity comes from the session token).

---

## 9. Open Questions

- **IdP integration specifics.** Which OAuth 2.0 flows, which token formats, how multi-IdP trust is established — TBD.
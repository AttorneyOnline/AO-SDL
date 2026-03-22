# Style Guide

This document describes the coding conventions used throughout the AO-SDL codebase. Formatting is enforced by clang-format (see `.clang-format`); this guide covers naming, structure, and idioms that a formatter can't enforce.

---

## Formatting (clang-format)

The `.clang-format` file is authoritative for whitespace and layout. Key settings:

- **Indentation:** 4 spaces, no tabs
- **Column limit:** 120
- **Braces:** Attach to statements (`if (...) {`), with `else` and `catch` on a new line after the closing brace
- **Pointers:** Left-aligned (`int* ptr`, not `int *ptr`)
- **Includes:** Sorted within groups, groups preserved as written
- **Lambdas:** Short lambdas allowed on a single line
- **Namespace bodies:** Not indented
- **Access modifiers:** Indented 2 spaces less than the body (so `public:` at 2-space indent, members at 4)

The pre-commit hook (`.githooks/pre-commit`) auto-formats staged `.cpp`, `.h`, and `.mm` files. Enable it with:

```sh
git config core.hooksPath .githooks
```

---

## Naming

### Types

- **Classes and structs:** PascalCase — `AssetCache`, `LayerGroup`, `TransformKeyframe`
- **Enum classes:** PascalCase — `Easing`, `EmoteMod`, `State`
- **Type aliases:** PascalCase — `using LruList = std::list<std::string>`

### Enum Values

- Enum class values that act as modes or states use PascalCase: `State::Preanim`, `State::Talking`
- Enum class values that act as named constants use UPPER_CASE: `Easing::QUAD_IN_OUT`, `EmoteMod::IDLE`

### Functions and Methods

- snake_case — `fetch_data()`, `evict_unused()`, `get_layer()`, `set_opacity()`
- Getters: `get_` prefix for value-returning accessors (`get_z_index()`, `get_opacity()`)
- Getters returning a reference to an internal object may omit the prefix: `transform()`, `asset()`, `state()`
- Setters: `set_` prefix (`set_opacity()`, `set_easing()`)
- Boolean queries: `is_` or `has_` prefix (`is_playing()`, `has_frame()`, `is_valid()`)

### Variables

- **Local variables and parameters:** snake_case — `delta_ms`, `cache_max_bytes`, `relative_path`
- **Private/protected member variables:** trailing underscore — `playing_`, `easing_`, `elapsed_ms_`, `shader_`, `transform_`
- **Private members that are simple data with no name collision:** may omit the underscore — `layers`, `lru`, `entries`, `preanim`, `idle`
  - Use the trailing underscore when the member name would collide with a method or parameter, or when the class has many members and the visual distinction aids readability
- **Public member variables:** no prefix or suffix — `valid`, `header`, `fields`, `conn_state`, `player_number`
- **Static constexpr constants:** UPPER_CASE — `DELIMITER`, `MIN_FIELDS`

### Files

- **Headers:** PascalCase matching the primary class — `AssetCache.h`, `GLRenderer.h`, `AOEmotePlayer.h`
- **Implementations:** Same name as header — `AssetCache.cpp`, `GLRenderer.cpp`
- **Platform files:** PascalCase — `SystemFonts.cpp`, `HardwareId.cpp`

---

## Header Layout

```cpp
#pragma once                                 // Always pragma once, never include guards

#include "project/Header.h"                  // Project headers first

#include <string>                            // Standard library headers second
#include <vector>

class ForwardDeclared;                       // Forward declarations after includes

/// Brief doc comment for the class.
class MyClass {
  public:
    // Constructors, destructor
    explicit MyClass(int value);

    // Public methods
    void do_something();
    int get_value() const { return value_; }

  private:
    // Private helpers
    void internal_step();

    // Data members
    int value_;
};
```

- `#pragma once` — never `#ifndef` guards
- Project includes first, then standard library includes, separated by a blank line
- Forward-declare rather than include when a pointer or reference is sufficient
- `public:` before `private:`
- Access specifiers indented at 2 spaces (clang-format enforces this)

---

## Inline Getters

Short accessors can be defined inline in the header:

```cpp
size_t used_bytes() const { return used_bytes_; }
bool is_playing() const { return playing_; }
const std::shared_ptr<ImageAsset>& get_asset() const { return asset; }
```

Anything beyond a trivial return goes in the `.cpp` file.

---

## Ownership and Lifetime

- **`std::shared_ptr<T>`** for reference-counted assets that may be held by multiple owners (cache + caller, game thread + render snapshot)
- **`std::unique_ptr<T>`** for exclusive ownership (mounts, subsystem instances)
- **`const T&`** for non-owning function parameters
- **`T&&`** for sink parameters that will be moved into storage
- Raw pointers are non-owning observers — never `delete` a raw pointer

Prefer passing `const std::shared_ptr<T>&` over `std::shared_ptr<T>` by value when the callee doesn't need to extend lifetime.

---

## Error Handling

- **`std::optional<T>`** for expected "not found" results — `get_event()`, `fetch_data()`, `probe()`
- **`nullptr`** return for pointer-typed lookups that may miss — `get()`, `get_layer()`
- **Exceptions** for protocol violations and broken invariants — `PacketFormatException`, `ProtocolStateException`
- Don't throw for routine missing-asset cases; those happen regularly during HTTP streaming

---

## Const Correctness

- Mark methods `const` whenever they don't mutate logical state
- Use `const` references for input parameters
- Use `mutable` only for synchronization primitives (`mutable std::mutex mutex_`)

---

## Comments

- Use `//` line comments, not `/* */` blocks
- Comments explain **why**, not **what** — the code should be readable on its own
- Block comments above classes or complex methods use `///` (Doxygen-compatible) or `//` with blank-line separation:

```cpp
// LRU cache for loaded Asset objects.
//
// Callers receive shared_ptr<Asset>. As long as a caller holds that pointer,
// the asset is pinned in memory...
class AssetCache {
```

- Don't add comments to trivial getters, setters, or obvious code

---

## Namespaces

- No `using namespace std;` — always qualify (`std::string`, `std::vector`, etc.)
- Namespace bodies are not indented (clang-format enforces this)
- Type aliases (`using IniDocument = ...`) are declared at namespace or class scope, not inside function bodies

---

## Templates

- Use `static_assert` to enforce constraints on template parameters:

```cpp
static_assert(std::is_base_of<Event, T>::value, "EventChannel only supports Event objects");
```

---

## Constructors

- Mark single-argument constructors `explicit`
- Use member initializer lists, not assignment in the constructor body
- Use in-class default initialization for simple defaults: `int elapsed_ms_ = 0;`, `float opacity = 1.0f;`
- Delete copy constructor/assignment when the type should not be copied:

```cpp
EventChannel(const EventChannel&) = delete;
EventChannel& operator=(const EventChannel&) = delete;
```

---

## Struct vs Class

- **`struct`** for plain data aggregates with all-public members — `TransformKeyframe`, `CacheEntry`, `ImageFrame`
- **`class`** for types with invariants, private state, or non-trivial methods

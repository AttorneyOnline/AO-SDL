# AO SDL
## An Attorney Online client, implemented from base principles

AO SDL is a reimplementation of AO2 using SDL2, OpenGL, and imgui. Focus is on correctness, performance, and modularized code.

See [Architecture.md](Architecture.md) for a detailed overview of the design principles, component architecture, and threading model. See [STYLE.md](STYLE.md) for coding conventions and naming rules.

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

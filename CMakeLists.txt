cmake_minimum_required(VERSION 3.14)
project(aosdl LANGUAGES CXX C)

if(MSVC)
    set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} /Zi")
    set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} /Zi")
endif()

if(WIN32)
    add_compile_definitions(WIN32_LEAN_AND_MEAN NOMINMAX)
endif()

# LTO for Release builds (significantly reduces binary size).
# Disabled on MSVC: LTCG's global code generation merges TUs with conflicting
# /Gs flags (SDL2 sets /Gs1048576 to avoid CRT deps).  The merged functions
# lose __chkstk stack probes, causing stack overflow crashes on Windows threads
# whose committed stack is smaller than the inlined frame.  See README.md.
if(NOT MSVC AND (CMAKE_BUILD_TYPE STREQUAL "Release" OR CMAKE_BUILD_TYPE STREQUAL "MinSizeRel"))
    include(CheckIPOSupported)
    check_ipo_supported(RESULT IPO_SUPPORTED)
    if(IPO_SUPPORTED)
        set(CMAKE_INTERPROCEDURAL_OPTIMIZATION ON)
        message(STATUS "LTO enabled")
    endif()
endif()

# Dead code elimination for MinSizeRel builds.
# -ffunction-sections/-fdata-sections put each symbol in its own section;
# -Wl,-dead_strip (macOS) or -Wl,--gc-sections (Linux) discards unreferenced ones.
if(CMAKE_BUILD_TYPE STREQUAL "MinSizeRel" AND NOT MSVC)
    add_compile_options(-ffunction-sections -fdata-sections)
    if(APPLE)
        add_link_options(-Wl,-dead_strip)
    else()
        add_link_options(-Wl,--gc-sections)
    endif()
    message(STATUS "Dead code elimination enabled")
endif()

# Strip executables in MinSizeRel builds (post-link step).
# Call aosdl_strip(target) on executable targets to add the post-build step.
if(CMAKE_BUILD_TYPE STREQUAL "MinSizeRel" AND NOT MSVC)
    find_program(STRIP_EXECUTABLE strip)
    if(STRIP_EXECUTABLE)
        function(aosdl_strip target)
            # Linker map: per-target to avoid clobbering when multiple exes build
            if(APPLE)
                target_link_options(${target} PRIVATE "LINKER:-map,$<TARGET_FILE:${target}>.map")
            else()
                target_link_options(${target} PRIVATE "LINKER:-Map,$<TARGET_FILE:${target}>.map")
            endif()
            add_custom_command(TARGET ${target} POST_BUILD
                COMMAND ${STRIP_EXECUTABLE} "$<TARGET_FILE:${target}>"
                COMMENT "Stripping ${target}")
        endfunction()
        message(STATUS "Strip + linker map enabled (post-build)")
    else()
        function(aosdl_strip target)
        endfunction()
        message(WARNING "strip not found — binaries will not be stripped")
    endif()
else()
    function(aosdl_strip target)
    endfunction()
endif()

# CMP0069 NEW: honor INTERPROCEDURAL_OPTIMIZATION on all targets.
# CMAKE_POLICY_DEFAULT propagates into subdirectories (unlike cmake_policy
# which is scope-local), so bundled SDL2, freetype, etc. participate in LTO.
set(CMAKE_POLICY_DEFAULT_CMP0069 NEW)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Server-only builds skip the SDL client, renderers, and their dependencies.
option(AO_BUILD_SERVER_ONLY "Build only the Kagami server (no SDL client, renderer, or GUI)" OFF)

# =============================================================================
# Third-party dependencies
# =============================================================================

# --- SDL2 — only needed for the desktop SDL app, not for Flutter/mobile ------
if(NOT AO_BUILD_FLUTTER AND NOT AO_BUILD_SERVER_ONLY)
    set(SDL_SHARED OFF CACHE BOOL "" FORCE)
    set(SDL_STATIC ON CACHE BOOL "" FORCE)
    set(SDL_JOYSTICK OFF CACHE BOOL "" FORCE)
    set(SDL_HAPTIC OFF CACHE BOOL "" FORCE)
    set(SDL_HIDAPI OFF CACHE BOOL "" FORCE)
    set(SDL_SENSOR OFF CACHE BOOL "" FORCE)
    set(SDL_LOCALE OFF CACHE BOOL "" FORCE)
    set(SDL_DISKAUDIO OFF CACHE BOOL "" FORCE)
    set(SDL_DUMMYAUDIO OFF CACHE BOOL "" FORCE)
    set(SDL_DUMMYVIDEO OFF CACHE BOOL "" FORCE)
    set(SDL_OFFSCREEN OFF CACHE BOOL "" FORCE)
    set(SDL_DIRECTFB OFF CACHE BOOL "" FORCE)
    set(SDL_RPI OFF CACHE BOOL "" FORCE)
    set(SDL_VIVANTE OFF CACHE BOOL "" FORCE)
    set(SDL_VULKAN OFF CACHE BOOL "" FORCE)
    set(SDL_RENDER_D3D OFF CACHE BOOL "" FORCE)
    set(SDL_TEST OFF CACHE BOOL "" FORCE)
    set(SDL_TESTS OFF CACHE BOOL "" FORCE)
    set(SDL2_DISABLE_INSTALL ON CACHE BOOL "" FORCE)
    set(SDL2_DISABLE_UNINSTALL ON CACHE BOOL "" FORCE)
    add_subdirectory(third-party/SDL)
endif()

# --- bit7z — archive extraction only ----------------------------------------
set(BIT7Z_AUTO_FORMAT ON CACHE BOOL "" FORCE)
set(BIT7Z_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(BIT7Z_BUILD_DOCS OFF CACHE BOOL "" FORCE)
add_subdirectory(third-party/bit7z)
# bit7z's OutputOptions.cmake writes its library to third-party/bit7z/lib/arm64/
# using PROJECT_SOURCE_DIR, which means iOS and macOS builds clobber each other.
# Override the target's output directory to go into the build tree instead.
set_target_properties(bit7z64 PROPERTIES
    ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib"
    LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib"
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
)

# --- libwebp — decode only, no tools or encoding features -------------------
set(WEBP_NEAR_LOSSLESS OFF CACHE BOOL "" FORCE)
set(WEBP_BUILD_ANIM_UTILS OFF CACHE BOOL "" FORCE)
set(WEBP_BUILD_CWEBP OFF CACHE BOOL "" FORCE)
set(WEBP_BUILD_DWEBP OFF CACHE BOOL "" FORCE)
set(WEBP_BUILD_GIF2WEBP OFF CACHE BOOL "" FORCE)
set(WEBP_BUILD_IMG2WEBP OFF CACHE BOOL "" FORCE)
set(WEBP_BUILD_VWEBP OFF CACHE BOOL "" FORCE)
set(WEBP_BUILD_WEBPINFO OFF CACHE BOOL "" FORCE)
set(WEBP_BUILD_LIBWEBPMUX OFF CACHE BOOL "" FORCE)
set(WEBP_BUILD_WEBPMUX OFF CACHE BOOL "" FORCE)
set(WEBP_BUILD_EXTRAS OFF CACHE BOOL "" FORCE)
add_subdirectory(third-party/libwebp)

# --- FreeType — font rasterization only, no optional deps --------------------
set(FT_DISABLE_HARFBUZZ ON CACHE BOOL "" FORCE)
set(FT_DISABLE_BZIP2 ON CACHE BOOL "" FORCE)
set(FT_DISABLE_BROTLI ON CACHE BOOL "" FORCE)
set(FT_DISABLE_PNG ON CACHE BOOL "" FORCE)
set(SKIP_INSTALL_ALL ON CACHE BOOL "" FORCE)
add_subdirectory(third-party/freetype)

# --- Ogg — container library, no extras --------------------------------------
set(BUILD_TESTING OFF CACHE BOOL "" FORCE)
set(INSTALL_DOCS OFF CACHE BOOL "" FORCE)
set(INSTALL_PKG_CONFIG_MODULE OFF CACHE BOOL "" FORCE)
set(INSTALL_CMAKE_PACKAGE_MODULE OFF CACHE BOOL "" FORCE)
add_subdirectory(third-party/ogg)

# --- Opus — codec library, no programs or tests -----------------------------
set(OPUS_BUILD_PROGRAMS OFF CACHE BOOL "" FORCE)
set(OPUS_BUILD_TESTING OFF CACHE BOOL "" FORCE)
set(OPUS_INSTALL_PKG_CONFIG_MODULE OFF CACHE BOOL "" FORCE)
set(OPUS_INSTALL_CMAKE_CONFIG_MODULE OFF CACHE BOOL "" FORCE)
add_subdirectory(third-party/opus)

# Opusfile — built manually since its CMakeLists uses find_package()
# which doesn't work with our source-built ogg/opus.
add_library(opusfile STATIC
    third-party/opusfile/src/info.c
    third-party/opusfile/src/internal.c
    third-party/opusfile/src/opusfile.c
    third-party/opusfile/src/stream.c
)
target_include_directories(opusfile
    PUBLIC third-party/opusfile/include
)
target_link_libraries(opusfile PUBLIC Ogg::ogg Opus::opus)
target_compile_definitions(opusfile PRIVATE OP_DISABLE_HTTP)

# ImGui (static lib — desktop only, not needed for Flutter or server)
if(NOT AO_BUILD_FLUTTER AND NOT AO_BUILD_SERVER_ONLY)
    set(IMGUI_DIR ${CMAKE_CURRENT_SOURCE_DIR}/third-party/imgui)
    set(IMGUI_SOURCES
        ${IMGUI_DIR}/imgui.cpp
        ${IMGUI_DIR}/imgui_draw.cpp
        ${IMGUI_DIR}/imgui_tables.cpp
        ${IMGUI_DIR}/imgui_widgets.cpp
        ${IMGUI_DIR}/backends/imgui_impl_sdl2.cpp
    )

    if(APPLE)
        list(APPEND IMGUI_SOURCES ${IMGUI_DIR}/backends/imgui_impl_metal.mm)
    else()
        find_package(OpenGL REQUIRED)
        list(APPEND IMGUI_SOURCES ${IMGUI_DIR}/backends/imgui_impl_opengl3.cpp)
    endif()

    add_library(imgui STATIC ${IMGUI_SOURCES})
    target_compile_definitions(imgui PUBLIC IMGUI_USE_WCHAR32)
    target_include_directories(imgui PUBLIC
        ${IMGUI_DIR}
        ${IMGUI_DIR}/backends
    )
    target_link_libraries(imgui PRIVATE SDL2::SDL2)

    if(APPLE)
        target_link_libraries(imgui PRIVATE
            "-framework Metal"
            "-framework MetalKit"
            "-framework QuartzCore"
        )
    else()
        target_link_libraries(imgui PRIVATE ${OPENGL_LIBRARIES})
        if(WIN32)
            target_include_directories(imgui PRIVATE third-party/glew/include)
        endif()
    endif()
endif()

# =============================================================================
# Version string from git
# =============================================================================

execute_process(
    COMMAND git rev-parse --short HEAD
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    OUTPUT_VARIABLE GIT_HASH
    OUTPUT_STRIP_TRAILING_WHITESPACE
    ERROR_QUIET
)
execute_process(
    COMMAND git status --porcelain
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    OUTPUT_VARIABLE GIT_STATUS
    OUTPUT_STRIP_TRAILING_WHITESPACE
    ERROR_QUIET
)

if(GIT_HASH STREQUAL "")
    set(AO_SDL_VERSION "unknown")
elseif(NOT GIT_STATUS STREQUAL "")
    set(AO_SDL_VERSION "${GIT_HASH}-dirty")
else()
    set(AO_SDL_VERSION "${GIT_HASH}")
endif()

configure_file(
    ${CMAKE_SOURCE_DIR}/include/utils/Version.h.in
    ${CMAKE_BINARY_DIR}/generated/utils/Version.h
    @ONLY
)

message(STATUS "AO-SDL version: ${AO_SDL_VERSION}")

# =============================================================================
# Embed assets/ directory into the binary
# =============================================================================
include(cmake/EmbedAssets.cmake)
embed_assets(EMBEDDED_ASSETS_SRC
    "${CMAKE_SOURCE_DIR}/assets"
    "${CMAKE_BINARY_DIR}/generated/EmbeddedAssets.cpp"
)

# =============================================================================
# Schema generation from OpenAPI spec (optional, requires Python3 + PyYAML)
# =============================================================================
option(AOSDL_GENERATE_SCHEMAS "Generate JSON schema validators from openapi.yaml (requires Python3 + PyYAML)" ON)

if(AOSDL_GENERATE_SCHEMAS AND NOT AO_BUILD_FLUTTER)
    find_package(Python3 COMPONENTS Interpreter)
    if(NOT Python3_FOUND)
        message(FATAL_ERROR
            "\n"
            "  ╔══════════════════════════════════════════════════════════════╗\n"
            "  ║  AOSDL_GENERATE_SCHEMAS=ON but Python3 was not found!      ║\n"
            "  ║  Install Python 3 and PyYAML, or pass:                     ║\n"
            "  ║    -DAOSDL_GENERATE_SCHEMAS=OFF                            ║\n"
            "  ╚══════════════════════════════════════════════════════════════╝\n"
        )
    else()
        # Verify PyYAML is available
        execute_process(
            COMMAND ${Python3_EXECUTABLE} -c "import yaml"
            RESULT_VARIABLE _pyyaml_result
            OUTPUT_QUIET ERROR_QUIET
        )
        if(NOT _pyyaml_result EQUAL 0)
            message(FATAL_ERROR
                "\n"
                "  ╔══════════════════════════════════════════════════════════════╗\n"
                "  ║  AOSDL_GENERATE_SCHEMAS=ON but PyYAML is not installed!    ║\n"
                "  ║  Install with: pip install pyyaml                           ║\n"
                "  ║  Or pass: -DAOSDL_GENERATE_SCHEMAS=OFF                     ║\n"
                "  ╚══════════════════════════════════════════════════════════════╝\n"
            )
        else()
            set(AONX_OPENAPI_SPEC "${CMAKE_SOURCE_DIR}/doc/aonx/openapi.yaml")
            set(AONX_SCHEMAS_CPP "${CMAKE_BINARY_DIR}/generated/AonxSchemas.cpp")

            add_custom_command(
                OUTPUT  "${AONX_SCHEMAS_CPP}"
                COMMAND ${Python3_EXECUTABLE}
                        "${CMAKE_SOURCE_DIR}/scripts/generate_schemas.py"
                        "${AONX_OPENAPI_SPEC}"
                        "${AONX_SCHEMAS_CPP}"
                DEPENDS "${AONX_OPENAPI_SPEC}"
                        "${CMAKE_SOURCE_DIR}/scripts/generate_schemas.py"
                COMMENT "Generating JSON schema validators from openapi.yaml"
            )
            set(AOSDL_SCHEMAS_AVAILABLE ON)
            message(STATUS "Schema generation enabled (Python ${Python3_VERSION})")
        endif()
    endif()
endif()

# --- sqlite3 — embedded SQL database (amalgamation build) -------------------
add_library(sqlite3 STATIC third-party/sqlite3/sqlite3.c)
set_property(TARGET sqlite3 PROPERTY C_STANDARD 11)
target_include_directories(sqlite3 PUBLIC third-party/sqlite3)
target_compile_definitions(sqlite3 PRIVATE
    SQLITE_THREADSAFE=2          # multi-thread mode (no shared connections)
    SQLITE_DEFAULT_WAL_SYNCHRONOUS=1  # NORMAL sync in WAL mode (safe + fast)
    SQLITE_DQS=0                 # disable double-quoted strings as values
    SQLITE_OMIT_DECLTYPE         # trim unused API surface
    SQLITE_OMIT_DEPRECATED       # trim deprecated APIs
    SQLITE_OMIT_PROGRESS_CALLBACK
    SQLITE_OMIT_SHARED_CACHE     # shared cache is deprecated
    SQLITE_DEFAULT_MEMSTATUS=0   # skip per-malloc bookkeeping
)

# =============================================================================
# aoengine — core engine library (no GL, no SDL)
#
# Public include root: engine/
#   #include "aoengine.h"          — umbrella header
#   #include "net/ProtocolHandler.h" — protocol plugin interface
#   #include "render/IRenderer.h"    — render backend interface
#   #include "ui/IUIRenderer.h"      — UI backend interface
# =============================================================================

# --- nghttp2 — HTTP/2 C library (static, lib sources only) -------------------
# Generate nghttp2ver.h from template (not committed in the nghttp2 repo).
set(PACKAGE_VERSION "1.68.90")
set(PACKAGE_VERSION_NUM 0x01445a)
configure_file(
    third-party/nghttp2/lib/includes/nghttp2/nghttp2ver.h.in
    ${CMAKE_CURRENT_BINARY_DIR}/nghttp2/nghttp2/nghttp2ver.h
    @ONLY
)
file(GLOB NGHTTP2_SOURCES third-party/nghttp2/lib/*.c)
add_library(nghttp2 STATIC ${NGHTTP2_SOURCES})
set_property(TARGET nghttp2 PROPERTY C_STANDARD 11)
target_include_directories(nghttp2
    PUBLIC  third-party/nghttp2/lib/includes
    PUBLIC  ${CMAKE_CURRENT_BINARY_DIR}/nghttp2  # for generated nghttp2ver.h
    PRIVATE third-party/nghttp2/lib
)
target_compile_definitions(nghttp2
    PRIVATE BUILDING_NGHTTP2
    PUBLIC  NGHTTP2_STATICLIB    # suppress __declspec(dllimport) in nghttp2.h for static linking
            NGHTTP2_NO_SSIZE_T  # consumers use *2 APIs (nghttp2_ssize, not ssize_t)
)
# MSVC lacks POSIX ssize_t — nghttp2's deprecated wrappers need it internally.
# BUILDING_NGHTTP2 undefines NGHTTP2_NO_SSIZE_T inside the library, exposing
# the ssize_t typedefs. Defining ssize_t as ptrdiff_t is the nghttp2-recommended
# approach (see nghttp2's CMakeLists.txt check_type_size block).
if(MSVC)
    target_compile_definitions(nghttp2 PRIVATE "ssize_t=ptrdiff_t")
endif()

add_library(aoengine STATIC
    engine/event/Event.cpp
    engine/event/ChatEvent.cpp
    engine/event/OutgoingChatEvent.cpp
    engine/event/UIEvent.cpp
    engine/event/ServerListEvent.cpp
    engine/event/CharacterListEvent.cpp
    engine/event/CharsCheckEvent.cpp
    engine/event/CharSelectRequestEvent.cpp
    engine/event/ServerConnectEvent.cpp
    engine/event/BackgroundEvent.cpp

    engine/utils/Log.cpp
    engine/utils/Base64.cpp
    engine/utils/JsonSchema.cpp
    engine/utils/JsonValidation.cpp
    engine/utils/StringHelpers.cpp

    engine/game/ASNReputationManager.cpp
    engine/game/BanManager.cpp
    engine/game/CommandRegistry.cpp
    engine/game/DatabaseManager.cpp
    engine/game/FirewallManager.cpp
    engine/game/GameRoom.cpp
    engine/game/IPReputationService.cpp
    engine/game/SpamDetector.cpp
    engine/game/GameThread.cpp
    engine/game/ServerList.cpp
    engine/game/Session.cpp

    engine/asset/AssetLibrary.cpp
    engine/asset/formats/ApngDecoder.cpp
    engine/asset/AssetCache.cpp
    engine/asset/MediaManager.cpp
    engine/asset/MountManager.cpp
    engine/asset/MountArchive.cpp
    engine/asset/MountDirectory.cpp
    engine/asset/MountEmbedded.cpp
    ${EMBEDDED_ASSETS_SRC}
    engine/asset/stb_image_impl.cpp

    engine/net/WebSocket.cpp
    engine/net/WebSocketFrame.cpp
    engine/net/WebSocketCommon.cpp
    engine/net/WebSocketServer.cpp
    engine/net/Http.cpp
    engine/net/HttpServer.cpp
    engine/net/ProxyProtocol.cpp
    engine/net/RateLimiter.cpp
    engine/net/TrustedProxyList.cpp
    engine/net/PlatformTcpSocket.cpp
    engine/net/PlatformServerSocket.cpp
    engine/net/WSClientThread.cpp

    engine/event/ICMessageEvent.cpp
    engine/event/OutgoingICMessageEvent.cpp
    engine/event/DisconnectEvent.cpp
    engine/ui/UIManager.cpp

    engine/render/Transform.cpp
    engine/render/TransformAnimator.cpp
    engine/render/StateBuffer.cpp
    engine/render/RenderState.cpp
    engine/render/RenderManager.cpp
    engine/render/Layer.cpp
    engine/render/AnimationPlayer.cpp
    engine/render/TextRenderer.cpp
    engine/render/GlyphCache.cpp
    engine/render/TextMeshBuilder.cpp
    engine/net/HttpPool.cpp
    engine/net/Http2Connection.cpp
    engine/net/EndpointFactory.cpp
    engine/net/RestRouter.cpp
    engine/asset/MountHttp.cpp
    engine/asset/ImageDecoders.cpp
    engine/asset/formats/WebPDecoder.cpp
    engine/asset/formats/GifDecoder.cpp
    engine/asset/formats/StbiDecoder.cpp
    engine/asset/miniaudio_impl.cpp
    engine/asset/AudioDecoders.cpp
    engine/asset/formats/MiniaudioDecoder.cpp
    engine/asset/formats/OpusDecoder.cpp
    engine/audio/AudioThread.cpp
    engine/audio/AudioStream.cpp
)

# Platform-specific implementations (selected at build time)
if(APPLE)
    target_sources(aoengine PRIVATE
        platform/macos/SystemFonts.mm
        platform/macos/HardwareId.mm
        platform/macos/Time.cpp
        platform/macos/Socket.mm
        platform/macos/Poll.cpp
    )
elseif(WIN32)
    target_sources(aoengine PRIVATE
        platform/windows/SystemFonts.cpp
        platform/windows/HardwareId.cpp
        platform/windows/Time.cpp
        platform/windows/Socket.cpp
        platform/windows/Poll.cpp
    )
else()
    # io_uring: completion-mode I/O backend (Linux 5.7+, liburing).
    # Eliminates per-connection recv() syscalls from the poll thread.
    # Disable with -DAO_USE_IO_URING=OFF to fall back to epoll.
    option(AO_USE_IO_URING "Use io_uring instead of epoll for I/O (Linux 5.7+)" ON)

    target_sources(aoengine PRIVATE
        platform/linux/SystemFonts.cpp
        platform/linux/HardwareId.cpp
        platform/linux/Time.cpp
        platform/linux/Socket.cpp
    )
    if(AO_USE_IO_URING)
        find_library(LIBURING_LIB uring)
        find_path(LIBURING_INCLUDE liburing.h)
        if(LIBURING_LIB AND LIBURING_INCLUDE)
            target_sources(aoengine PRIVATE platform/linux/Poll_uring.cpp)
            target_include_directories(aoengine PRIVATE ${LIBURING_INCLUDE})
            target_link_libraries(aoengine PRIVATE ${LIBURING_LIB})
            target_compile_definitions(aoengine PUBLIC AO_HAS_IO_URING=1)
            message(STATUS "I/O backend: io_uring (liburing found at ${LIBURING_LIB})")
        else()
            message(WARNING "AO_USE_IO_URING=ON but liburing not found — falling back to epoll")
            target_sources(aoengine PRIVATE platform/linux/Poll.cpp)
        endif()
    else()
        target_sources(aoengine PRIVATE platform/linux/Poll.cpp)
        message(STATUS "I/O backend: epoll (io_uring disabled)")
    endif()
endif()

set_property(TARGET aoengine PROPERTY CXX_STANDARD 20)

target_include_directories(aoengine
    PUBLIC  include
            ${CMAKE_BINARY_DIR}/generated
            third-party/header-libs
    PRIVATE engine
            third-party/stb
            third-party/bit7z
)

# TLS support: Secure Transport on Apple, OpenSSL on Linux/Windows
if(APPLE)
    target_link_libraries(aoengine PUBLIC bit7z freetype webpdemux webp opusfile nghttp2 sqlite3)
    set_source_files_properties(platform/macos/Socket.mm PROPERTIES COMPILE_FLAGS "-fobjc-arc")
else()
    set(OPENSSL_USE_STATIC_LIBS ON)
    find_package(OpenSSL REQUIRED)
    target_link_libraries(aoengine PUBLIC bit7z freetype webpdemux webp opusfile nghttp2 sqlite3 OpenSSL::SSL OpenSSL::Crypto)
endif()
target_include_directories(aoengine PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/third-party/libwebp/src)

if(APPLE)
    target_link_libraries(aoengine PRIVATE
        "-framework CoreText"
        "-framework Foundation"
        "-framework IOKit"
        "-framework Network"
        "-framework Security"
    )
endif()

# =============================================================================
# Plugins
# =============================================================================

if(NOT AO_BUILD_SERVER_ONLY)
if(APPLE)
    # --- aorender_metal — Metal renderer backend (macOS / iOS) ----------------

    add_library(aorender_metal STATIC
        plugins/render/metal/MetalRenderer.mm
        plugins/render/metal/MetalTexture.mm
        plugins/render/metal/Texture.mm
    )
    set_property(TARGET aorender_metal PROPERTY CXX_STANDARD 20)

    target_include_directories(aorender_metal
        PUBLIC  plugins
        PRIVATE plugins/render/metal
    )
    target_link_libraries(aorender_metal PUBLIC aoengine
        "-framework Metal"
        "-framework MetalKit"
        "-framework QuartzCore"
        "-framework Foundation"
    )

else()
    # --- aorender_gl — OpenGL renderer backend --------------------------------

    add_library(aorender_gl STATIC
        plugins/render/gl/Texture.cpp
        plugins/render/gl/Shader.cpp
        plugins/render/gl/GLTexture.cpp
        plugins/render/gl/GLMesh.cpp
        plugins/render/gl/GLSprite.cpp
        plugins/render/gl/GLRenderer.cpp
    )
    set_property(TARGET aorender_gl PROPERTY CXX_STANDARD 20)

    target_include_directories(aorender_gl
        PUBLIC  plugins
        PRIVATE plugins/render/gl
    )
    target_link_libraries(aorender_gl PUBLIC aoengine ${OPENGL_LIBRARIES})

    if(WIN32)
        target_include_directories(aorender_gl PUBLIC third-party/glew/include)
        target_link_libraries(aorender_gl PUBLIC
            ${CMAKE_CURRENT_SOURCE_DIR}/third-party/glew/lib/Release/x64/glew32.lib)
    else()
        find_package(GLEW REQUIRED)
        target_include_directories(aorender_gl PUBLIC ${GLEW_INCLUDE_DIRS})
        target_link_libraries(aorender_gl PUBLIC ${GLEW_LIBRARIES})
    endif()
endif()
endif() # NOT AO_BUILD_SERVER_ONLY

# --- ao_net — AO2 network protocol (packet parsing, serialization) -----------

add_library(ao_net STATIC
    plugins/net/ao/AOClient.cpp
    plugins/net/ao/AOServer.cpp
    plugins/net/ao/AOPacket.cpp
    plugins/net/ao/PacketTypes.cpp
    plugins/net/ao/PacketBehavior.cpp
    plugins/net/ao/ServerPacketBehavior.cpp
    plugins/net/ao/commands/RegisterCommands.cpp
    plugins/net/ao/commands/HelpCommand.cpp
    plugins/net/ao/commands/MotdCommand.cpp
    plugins/net/ao/commands/OnlineCommand.cpp
    plugins/net/ao/commands/LoginCommand.cpp
    plugins/net/ao/commands/LogoutCommand.cpp
    plugins/net/ao/commands/KickCommand.cpp
    plugins/net/ao/commands/AsnCommand.cpp
    plugins/net/ao/commands/BanCommand.cpp
    plugins/net/ao/commands/EditBanCommand.cpp
    plugins/net/ao/commands/SearchBansCommand.cpp
    plugins/net/ao/commands/FirewallCommand.cpp
    plugins/net/ao/commands/ReputationCommand.cpp
    plugins/net/ao/commands/UnbanCommand.cpp
    plugins/net/ao/commands/ChangeAuthCommand.cpp
    plugins/net/ao/commands/RootPassCommand.cpp
    plugins/net/ao/commands/AddUserCommand.cpp
    plugins/net/ao/commands/RemoveUserCommand.cpp
    plugins/net/ao/commands/ListUsersCommand.cpp
    plugins/net/ao/commands/SetPermsCommand.cpp
    plugins/net/ao/commands/RemovePermsCommand.cpp
    plugins/net/ao/commands/ListPermsCommand.cpp
    plugins/net/ao/commands/ChangePassCommand.cpp
    plugins/net/ao/commands/AreaCommand.cpp
    plugins/net/ao/commands/GetAreaCommand.cpp
    plugins/net/ao/commands/BackgroundCommand.cpp
    plugins/net/ao/commands/StatusCommand.cpp
    plugins/net/ao/commands/AreaLockCommand.cpp
    plugins/net/ao/commands/CmCommand.cpp
    plugins/net/ao/commands/TestimonyCommand.cpp
)
set_property(TARGET ao_net PROPERTY CXX_STANDARD 20)
target_include_directories(ao_net
    PUBLIC  plugins
    PRIVATE plugins/net/ao   # bare includes within the net plugin
)
target_link_libraries(ao_net PUBLIC aoengine)

# --- nx_net — AONX network protocol (JSON messages, REST+WebSocket) ---------

add_library(nx_net STATIC
    plugins/net/nx/NXMessage.cpp
    plugins/net/nx/NXServer.cpp
    plugins/net/nx/NXEndpoint.cpp
    plugins/net/nx/endpoints/ServerEndpoint.cpp
    plugins/net/nx/endpoints/SessionCreateEndpoint.cpp
    plugins/net/nx/endpoints/SessionDeleteEndpoint.cpp
    plugins/net/nx/endpoints/SessionRenewEndpoint.cpp
    plugins/net/nx/endpoints/CharacterListEndpoint.cpp
    plugins/net/nx/endpoints/CharacterGetEndpoint.cpp
    plugins/net/nx/endpoints/CharacterSelectEndpoint.cpp
    plugins/net/nx/endpoints/AreaListEndpoint.cpp
    plugins/net/nx/endpoints/AreaGetEndpoint.cpp
    plugins/net/nx/endpoints/AreaPlayersEndpoint.cpp
    plugins/net/nx/endpoints/AreaJoinEndpoint.cpp
    plugins/net/nx/endpoints/AreaIcEndpoint.cpp
    plugins/net/nx/endpoints/AreaOocEndpoint.cpp
    plugins/net/nx/endpoints/AdminSessionsEndpoint.cpp
)
set_property(TARGET nx_net PROPERTY CXX_STANDARD 20)
target_include_directories(nx_net
    PUBLIC  plugins
    PRIVATE plugins/net/nx
)
target_link_libraries(nx_net PUBLIC aoengine)

if(AOSDL_SCHEMAS_AVAILABLE)
    target_sources(nx_net PRIVATE "${AONX_SCHEMAS_CPP}")
    target_compile_definitions(nx_net PUBLIC AOSDL_HAS_GENERATED_SCHEMAS=1)
else()
    target_sources(nx_net PRIVATE engine/utils/GeneratedSchemasStub.cpp)
endif()

# --- kagami_server — server orchestration (protocol routing, room management) -

add_library(kagami_server INTERFACE)
target_include_directories(kagami_server INTERFACE plugins)
target_link_libraries(kagami_server INTERFACE aoengine ao_net nx_net)

# --- ao_game — AO2 game logic (courtroom, emotes, textbox, effects) ---------

if(NOT AO_BUILD_SERVER_ONLY)
add_library(ao_game STATIC
    plugins/ao/asset/AOAssetLibrary.cpp
    plugins/ao/asset/AOCharacterSheet.cpp
    plugins/ao/game/AOBlipPlayer.cpp
    plugins/ao/game/AOCourtroomPresenter.cpp
    plugins/ao/game/AOSceneCompositor.cpp
    plugins/ao/game/AOMessagePlayer.cpp
    plugins/ao/game/AOMusicPlayer.cpp
    plugins/ao/game/ICMessageQueue.cpp
    plugins/ao/game/AOBackground.cpp
    plugins/ao/game/AOEmotePlayer.cpp
    plugins/ao/game/AOTextBox.cpp
    plugins/ao/game/AOTextProcessor.cpp
    plugins/ao/game/effects/ScreenshakeEffect.cpp
    plugins/ao/game/effects/SlideEffect.cpp
    plugins/ao/game/effects/FlashEffect.cpp
    plugins/ao/game/effects/ShaderEffect.cpp
)
set_property(TARGET ao_game PROPERTY CXX_STANDARD 20)
target_include_directories(ao_game PUBLIC plugins)
target_link_libraries(ao_game PUBLIC aoengine)

# --- ao_protocol — thin glue linking net + game + UI screens -----------------

add_library(ao_protocol STATIC
    plugins/ao/ao_plugin.cpp
    plugins/ao/ui/screens/ServerListScreen.cpp
    plugins/ao/ui/screens/CharSelectScreen.cpp
    plugins/ao/ui/screens/CourtroomScreen.cpp
)
set_property(TARGET ao_protocol PROPERTY CXX_STANDARD 20)
target_include_directories(ao_protocol PUBLIC plugins)
target_link_libraries(ao_protocol PUBLIC ao_net ao_game)
endif() # NOT AO_BUILD_SERVER_ONLY

# =============================================================================
# Applications — one per backend
# =============================================================================

if(NOT AO_BUILD_FLUTTER AND NOT AO_BUILD_SERVER_ONLY)
    add_subdirectory(apps/sdl)
    # add_subdirectory(apps/qt)   # future: ENABLE_THE_HORRORS
endif()

if(NOT AO_BUILD_FLUTTER)
    add_subdirectory(apps/kagami)
endif()

# --- ao_bridge — C API for Flutter/Dart FFI (mobile) -------------------------
# Built as a static library; the Flutter runner links it in.
# Only included when AO_BUILD_FLUTTER is ON (set by the ios-flutter preset).

option(AO_BUILD_FLUTTER "Build the Flutter FFI bridge library" OFF)
if(AO_BUILD_FLUTTER)
    add_library(ao_bridge STATIC
        apps/flutter/native/bridge.cpp
        apps/flutter/native/MiniaudioDevice.cpp
        apps/flutter/native/miniaudio_device_impl.mm
    )
    # ObjC++ files need ARC for proper memory management
    set_source_files_properties(
        apps/flutter/native/miniaudio_device_impl.mm
        PROPERTIES COMPILE_FLAGS "-fobjc-arc"
    )
    set_property(TARGET ao_bridge PROPERTY CXX_STANDARD 20)
    target_include_directories(ao_bridge PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/include
        ${CMAKE_CURRENT_SOURCE_DIR}/plugins
        ${CMAKE_CURRENT_SOURCE_DIR}/apps/flutter/native
        ${CMAKE_BINARY_DIR}/generated
    )
    target_link_libraries(ao_bridge PRIVATE ao_protocol)
    if(APPLE)
        target_link_libraries(ao_bridge PRIVATE aorender_metal
            "-framework AudioToolbox"
            "-framework CoreAudio"
            "-framework AVFoundation"
        )
    else()
        target_link_libraries(ao_bridge PRIVATE aorender_gl)
    endif()

    # flutter-build: build the native bridge and all libraries that the
    # xcconfig links against (some are not transitive deps of ao_bridge).
    add_custom_target(flutter-build
        DEPENDS ao_bridge webpdecoder sharpyuv sqlite3
        COMMENT "Building native bridge for Flutter"
    )
endif()

# =============================================================================
# Tests
# =============================================================================

if(NOT AO_BUILD_FLUTTER AND NOT AO_BUILD_SERVER_ONLY)
    enable_testing()
    add_subdirectory(tests)
endif()

# =============================================================================
# API Documentation (Doxygen)
# =============================================================================

find_package(Doxygen QUIET)

if(DOXYGEN_FOUND)
    message(STATUS "Found Doxygen: ${DOXYGEN_EXECUTABLE}")

    add_custom_target(docs
        COMMAND ${DOXYGEN_EXECUTABLE} ${CMAKE_SOURCE_DIR}/Doxyfile
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
        COMMENT "Generating API documentation with Doxygen → docs/html/index.html"
        VERBATIM
    )
else()
    message(STATUS "Doxygen not found — the 'docs' target will not be available.")
endif()


# =============================================================================
# clang-tidy
# =============================================================================

find_program(CLANG_TIDY_EXE NAMES clang-tidy)

if(CLANG_TIDY_EXE)
    message(STATUS "Found clang-tidy: ${CLANG_TIDY_EXE}")

    add_custom_target(run-clang-tidy
        COMMAND ${CMAKE_COMMAND} -E echo "Running clang-tidy..."
        COMMAND /bin/sh -c "find ${CMAKE_SOURCE_DIR} -path '${CMAKE_SOURCE_DIR}/third-party' -prune -o -name '*.cpp' -print0 | xargs -0 ${CLANG_TIDY_EXE} -header-filter='.*[/\\](engine|plugins|apps)[/\\].*' -p ${CMAKE_BINARY_DIR}"
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
        COMMENT "Running clang-tidy on the project sources using compile_commands.json"
        VERBATIM
    )
else()
    message(WARNING "clang-tidy not found! The 'run-clang-tidy' target will not be available.")
endif()

cmake_minimum_required(VERSION 3.14)
project(aosdl LANGUAGES CXX C)

if(MSVC)
    set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} /Zi")
    set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} /Zi")
endif()

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# =============================================================================
# Third-party dependencies
# =============================================================================

add_subdirectory(third-party/SDL)

set(BIT7Z_AUTO_FORMAT ON CACHE BOOL "enable auto format support" FORCE)
add_subdirectory(third-party/bit7z)

set(FT_DISABLE_HARFBUZZ ON CACHE BOOL "" FORCE)
set(FT_DISABLE_BZIP2 ON CACHE BOOL "" FORCE)
set(FT_DISABLE_BROTLI ON CACHE BOOL "" FORCE)
set(FT_DISABLE_PNG ON CACHE BOOL "" FORCE)
add_subdirectory(third-party/freetype)

# ImGui (static lib — backend-specific sources selected below)
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

# =============================================================================
# aoengine — core engine library (no GL, no SDL)
#
# Public include root: engine/
#   #include "aoengine.h"          — umbrella header
#   #include "net/ProtocolHandler.h" — protocol plugin interface
#   #include "render/IRenderer.h"    — render backend interface
#   #include "ui/IUIRenderer.h"      — UI backend interface
# =============================================================================

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
    engine/utils/JsonValidation.cpp

    engine/game/GameThread.cpp
    engine/game/ServerList.cpp

    engine/asset/AssetLibrary.cpp
    engine/asset/ApngDecoder.cpp
    engine/asset/AssetCache.cpp
    engine/asset/MediaManager.cpp
    engine/asset/MountManager.cpp
    engine/asset/MountArchive.cpp
    engine/asset/MountDirectory.cpp
    engine/asset/stb_image_impl.cpp

    engine/net/WebSocket.cpp
    engine/net/KissnetTcpSocket.cpp
    engine/net/NetworkThread.cpp

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
)
set_property(TARGET aoengine PROPERTY CXX_STANDARD 20)

target_include_directories(aoengine
    PUBLIC  include
            third-party/header-libs
    PRIVATE engine
            third-party/stb
            third-party/kissnet
            third-party/bit7z
)

target_link_libraries(aoengine PUBLIC bit7z freetype)

# =============================================================================
# Plugins
# =============================================================================

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

# --- ao_protocol — AO2 protocol plugin ---------------------------------------

add_library(ao_protocol STATIC
    plugins/ao/ao_plugin.cpp
    plugins/ao/event/ICMessageEvent.cpp
    plugins/ao/event/OutgoingICMessageEvent.cpp
    plugins/ao/asset/AOAssetLibrary.cpp
    plugins/ao/asset/AOCharacterSheet.cpp
    plugins/ao/game/AOCourtroomPresenter.cpp
    plugins/ao/game/ICMessageQueue.cpp
    plugins/ao/game/AOBackground.cpp
    plugins/ao/game/AOEmotePlayer.cpp
    plugins/ao/game/AOTextBox.cpp
    plugins/ao/game/effects/ScreenshakeEffect.cpp
    plugins/ao/game/effects/FlashEffect.cpp
    plugins/ao/net/AOClient.cpp
    plugins/ao/net/AOPacket.cpp
    plugins/ao/net/PacketTypes.cpp
    plugins/ao/net/PacketBehavior.cpp
    plugins/ao/ui/screens/ServerListScreen.cpp
    plugins/ao/ui/screens/CharSelectScreen.cpp
    plugins/ao/ui/screens/CourtroomScreen.cpp
)
set_property(TARGET ao_protocol PROPERTY CXX_STANDARD 20)

target_include_directories(ao_protocol PUBLIC plugins)
target_link_libraries(ao_protocol PUBLIC aoengine)

# =============================================================================
# Applications — one per backend
# =============================================================================

add_subdirectory(apps/sdl)
# add_subdirectory(apps/qt)   # future: ENABLE_THE_HORRORS

# =============================================================================
# Tests
# =============================================================================

enable_testing()
add_subdirectory(tests)

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

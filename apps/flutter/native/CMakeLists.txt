# Build the C bridge as a shared library that Flutter loads via dart:ffi.
# This links against the same static libraries as apps/sdl but without
# SDL, ImGui, or any UI code.

cmake_minimum_required(VERSION 3.14)
project(ao_bridge C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Path to the AO-SDL repo root
get_filename_component(AO_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/../../.." ABSOLUTE)

add_library(ao_bridge SHARED
    bridge.cpp
)

target_include_directories(ao_bridge PRIVATE
    ${AO_ROOT}/include
    ${AO_ROOT}/plugins/ao/include
    ${AO_ROOT}/plugins/net/ao/include
    ${AO_ROOT}/third-party/json/include
)

# Link against the engine and plugin static libraries.
# These must already be built (or built as part of a superbuild).
# For now, reference the targets directly assuming this is included
# from a top-level CMake that builds everything.
target_link_libraries(ao_bridge PRIVATE
    aoengine
    ao_game
    ao_net
)

# On iOS, also link Metal renderer. On Android, link GL renderer.
if(APPLE)
    target_link_libraries(ao_bridge PRIVATE aorender_metal)
    target_link_libraries(ao_bridge PRIVATE
        "-framework Metal"
        "-framework MetalKit"
        "-framework QuartzCore"
    )
else()
    target_link_libraries(ao_bridge PRIVATE aorender_gl)
    target_link_libraries(ao_bridge PRIVATE GLESv3 EGL)
endif()

# Export all ao_* symbols
set_target_properties(ao_bridge PROPERTIES
    C_VISIBILITY_PRESET default
    CXX_VISIBILITY_PRESET default
)

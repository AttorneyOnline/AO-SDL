cmake_minimum_required(VERSION 3.3)
project(tsurushiage LANGUAGES CXX C)

set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} /Zi")
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} /Zi")

# Source files
set(SRC_FILES
    src/main.cpp
    src/ao/net/AOClient.cpp
    src/ao/net/AOPacket.cpp
    src/ao/net/PacketTypes.cpp
    src/ao/net/PacketBehavior.cpp
    src/event/Event.cpp
    src/event/ChatEvent.cpp
    src/event/UIEvent.cpp
    src/utils/Log.cpp
    src/utils/Base64.cpp
    src/video/GameWindow.cpp
    src/game/GameThread.cpp
    src/render/detail/Renderer.cpp
    src/render/detail/Shader.cpp
    src/render/detail/Sprite.cpp
    src/render/detail/Texture.cpp
    src/render/detail/Transform.cpp
    src/render/detail/Vertex.cpp
    src/render/StateBuffer.cpp
    src/render/RenderState.cpp
    src/render/RenderManager.cpp
    src/render/Image.cpp
    src/render/Layer.cpp
    src/asset/AssetLibrary.cpp
    src/asset/Asset.cpp
    src/asset/AssetCache.cpp
    src/asset/ImageAsset.cpp
    src/net/WebSocket.cpp
    src/net/NetworkThread.cpp
    src/ui/UIManager.cpp)

# Include directories
set(INCLUDE_DIRS
    src
    third-party/stb
    third-party/glm
    third-party/SDL/include
)

# Add executable
add_executable(${CMAKE_PROJECT_NAME} ${SRC_FILES})

# Specify C++ standard
set_property(TARGET ${CMAKE_PROJECT_NAME} PROPERTY CXX_STANDARD 20)

# Include directories for the project
target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE ${INCLUDE_DIRS})

# SDL2
add_subdirectory(third-party/SDL)
if(TARGET SDL2::SDL2main)
    target_link_libraries(${CMAKE_PROJECT_NAME} PRIVATE SDL2::SDL2main)
endif()
target_link_libraries(${CMAKE_PROJECT_NAME} PRIVATE SDL2::SDL2)

# OpenGL
find_package(OpenGL REQUIRED)
target_link_libraries(${CMAKE_PROJECT_NAME} PRIVATE ${OPENGL_LIBRARIES})

# GLEW (Windows-specific setup)
if(WIN32)
    # GLEW settings
    target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE third-party/glew/include)
    target_link_libraries(${CMAKE_PROJECT_NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/third-party/glew/lib/Release/x64/glew32.lib)

    # Copy DLLs to binary directory
    add_custom_command(
        TARGET ${CMAKE_PROJECT_NAME} POST_BUILD
        COMMAND "${CMAKE_COMMAND}" -E copy_if_different
            ${CMAKE_CURRENT_SOURCE_DIR}/third-party/glew/bin/Release/x64/glew32.dll
            "$<TARGET_FILE_DIR:${CMAKE_PROJECT_NAME}>"
        VERBATIM
    )
    add_custom_command(
        TARGET ${CMAKE_PROJECT_NAME} POST_BUILD
        COMMAND "${CMAKE_COMMAND}" -E copy_if_different
            "$<TARGET_FILE:SDL2::SDL2>"
            "$<TARGET_FILE_DIR:${CMAKE_PROJECT_NAME}>"
        VERBATIM
    )
endif()

# ImGui setup
set(IMGUI_DIR ${CMAKE_CURRENT_SOURCE_DIR}/third-party/imgui)
set(IMGUI_FILES
    ${IMGUI_DIR}/imgui.cpp
    ${IMGUI_DIR}/imgui_draw.cpp
    ${IMGUI_DIR}/imgui_tables.cpp
    ${IMGUI_DIR}/imgui_widgets.cpp
    ${IMGUI_DIR}/backends/imgui_impl_sdl2.cpp
    ${IMGUI_DIR}/backends/imgui_impl_opengl3.cpp
)

add_library(imgui STATIC ${IMGUI_FILES})
target_include_directories(imgui PUBLIC
    ${IMGUI_DIR}
    ${IMGUI_DIR}/backends
    ${INCLUDE_DIRS}
)
target_link_libraries(${CMAKE_PROJECT_NAME} PRIVATE imgui)

# Kissnet
target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/third-party/kissnet)
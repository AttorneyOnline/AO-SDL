cmake_minimum_required (VERSION 3.8)

set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} /Zi")
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} /Zi")

project ("tsurushiage" LANGUAGES CXX C)
add_executable ( ${CMAKE_PROJECT_NAME}
    "src/main.cpp"
    "src/utils/Log.cpp"
  "src/video/GameWindow.cpp" "src/game/GameThread.h" "src/game/GameThread.cpp"
"src/render/detail/Renderer.cpp"
    "src/render/detail/Shader.cpp"
    "src/render/detail/Sprite.cpp"
    "src/render/detail/Texture.cpp"
    "src/render/detail/Transform.cpp"
    "src/render/detail/Vertex.cpp"
    "src/render/StateBuffer.cpp"
    "src/render/RenderState.cpp"
    "src/render/RenderManager.cpp"
    "src/render/Image.cpp"
    "src/render/Layer.cpp")
set_property(TARGET ${CMAKE_PROJECT_NAME} PROPERTY CXX_STANDARD 20)

target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE src)

# STB utilities
target_include_directories(${PROJECT_NAME} PRIVATE third-party/stb)

# SDL2
add_subdirectory(third-party/SDL)
if(TARGET SDL2::SDL2main)
    target_link_libraries(${CMAKE_PROJECT_NAME} PRIVATE SDL2::SDL2main)
endif()
target_link_libraries(${CMAKE_PROJECT_NAME} PRIVATE SDL2::SDL2)

# GLM
target_include_directories(${PROJECT_NAME} PRIVATE third-party/glm)

# OpenGL
find_package(OpenGL)
target_link_libraries(${PROJECT_NAME} PRIVATE ${OPENGL_LIBRARIES})

if (WIN32)
    # Use GLEW release binaries on Windows because Windows sucks ass
    target_include_directories(${PROJECT_NAME} PRIVATE third-party/glew/include)
    target_link_libraries(${PROJECT_NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/third-party/glew/lib/Release/x64/glew32.lib)
endif()

if (WIN32)

    # Copy DLLs to binary location
    add_custom_command(
        TARGET ${CMAKE_PROJECT_NAME} POST_BUILD
        COMMAND "${CMAKE_COMMAND}" -E copy_if_different ${CMAKE_CURRENT_SOURCE_DIR}/third-party/glew/bin/Release/x64/glew32.dll "$<TARGET_FILE_DIR:${PROJECT_NAME}>"
        VERBATIM
    )
    add_custom_command(
        TARGET ${CMAKE_PROJECT_NAME} POST_BUILD
        COMMAND "${CMAKE_COMMAND}" -E copy_if_different "$<TARGET_FILE:SDL2::SDL2>" "$<TARGET_FILE_DIR:${PROJECT_NAME}>"
        VERBATIM
    )
endif()

include_directories(
    third-party/SDL/include
)

# imgui
set(IMGUI_DIR ${CMAKE_CURRENT_SOURCE_DIR}/third-party/imgui)

set(IMGUI_FILES
    ${IMGUI_DIR}/imgui.cpp
    ${IMGUI_DIR}/imgui_draw.cpp
    ${IMGUI_DIR}/imgui_tables.cpp
    ${IMGUI_DIR}/imgui_widgets.cpp
    ${IMGUI_DIR}/backends/imgui_impl_sdl2.cpp
    ${IMGUI_DIR}/backends/imgui_impl_opengl3.cpp
)

add_library(imgui ${IMGUI_FILES})
target_include_directories(imgui PUBLIC
    ${IMGUI_DIR}
    ${IMGUI_DIR}/backends
    # Add additional include directories for SDL and OpenGL if needed
)
target_link_libraries(${PROJECT_NAME} PRIVATE imgui)

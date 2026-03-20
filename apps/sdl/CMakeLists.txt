set(SDL_APP_SOURCES
    main.cpp
    SDLGameWindow.cpp
    ImGuiUIRenderer.cpp
)

if(APPLE)
    list(APPEND SDL_APP_SOURCES MetalBackend.mm)
else()
    list(APPEND SDL_APP_SOURCES GLBackend.cpp)
endif()

add_executable(aosdl ${SDL_APP_SOURCES})
set_property(TARGET aosdl PROPERTY CXX_STANDARD 20)

target_link_libraries(aosdl PRIVATE
    aoengine
    ao_protocol
    imgui
    SDL2::SDL2
)

if(APPLE)
    target_link_libraries(aosdl PRIVATE aorender_metal
        "-framework Metal"
        "-framework MetalKit"
        "-framework QuartzCore"
    )
else()
    target_link_libraries(aosdl PRIVATE aorender_gl)
endif()

if(TARGET SDL2::SDL2main)
    target_link_libraries(aosdl PRIVATE SDL2::SDL2main)
endif()

# Copy runtime DLLs on Windows
if(WIN32)
    add_custom_command(
        TARGET aosdl POST_BUILD
        COMMAND "${CMAKE_COMMAND}" -E copy_if_different
            ${CMAKE_SOURCE_DIR}/third-party/glew/bin/Release/x64/glew32.dll
            "$<TARGET_FILE_DIR:aosdl>"
        VERBATIM
    )
    add_custom_command(
        TARGET aosdl POST_BUILD
        COMMAND "${CMAKE_COMMAND}" -E copy_if_different
            "$<TARGET_FILE:SDL2::SDL2>"
            "$<TARGET_FILE_DIR:aosdl>"
        VERBATIM
    )
endif()

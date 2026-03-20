set(SDL_APP_SOURCES
    main.cpp
    SDLGameWindow.cpp
    ui/ImGuiUIRenderer.cpp
    ui/widgets/ChatWidget.cpp
    ui/widgets/ServerListWidget.cpp
    ui/widgets/CharSelectWidget.cpp
    ui/widgets/CourtroomWidget.cpp
    ui/widgets/ICChatWidget.cpp
    ui/widgets/EmoteSelectorWidget.cpp
    ui/widgets/InterjectionWidget.cpp
    ui/widgets/SideSelectWidget.cpp
    ui/widgets/MessageOptionsWidget.cpp
)

if(APPLE)
    list(APPEND SDL_APP_SOURCES render/MetalBackend.mm)
else()
    list(APPEND SDL_APP_SOURCES render/GLBackend.cpp)
endif()

add_executable(aosdl ${SDL_APP_SOURCES})
set_property(TARGET aosdl PROPERTY CXX_STANDARD 20)
target_include_directories(aosdl PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

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

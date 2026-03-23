find_package(Qt6 REQUIRED COMPONENTS Core Gui Quick OpenGL)

set(QT_APP_SOURCES
    main.cpp
    QtGameWindow.cpp
    renderer/GLBackend.cpp
)

add_executable(aoqml ${QT_APP_SOURCES})
target_include_directories(aoqml PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

set_property(TARGET aoqml PROPERTY CXX_STANDARD 20)
set_property(TARGET aoqml PROPERTY CXX_STANDARD_REQUIRED ON)

set_property(TARGET aoqml PROPERTY AUTOMOC ON)
set_property(TARGET aoqml PROPERTY AUTORCC ON)
set_property(TARGET aoqml PROPERTY AUTOUIC ON)

target_link_libraries(aoqml PRIVATE
    ao_protocol
    Qt::Core
    Qt::Gui
    Qt::Quick
    Qt::OpenGL
)

if(APPLE)
    target_link_libraries(aoqml PRIVATE aorender_metal
        "-framework Metal"
        "-framework MetalKit"
        "-framework QuartzCore"
    )
else()
    target_link_libraries(aoqml PRIVATE aorender_gl)
endif()

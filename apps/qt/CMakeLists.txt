find_package(Qt6 REQUIRED COMPONENTS Core Gui Qml Quick OpenGL)

# Qt 6.8+ requires explicit policy settings for qt_add_qml_module.
if(COMMAND qt_policy)
    qt_policy(SET QTP0001 NEW)
    qt_policy(SET QTP0004 NEW)
endif()

set(QT_APP_SOURCES
    main.cpp
    QtGameWindow.cpp
    CharIconProvider.h  CharIconProvider.cpp
    EmoteIconProvider.h EmoteIconProvider.cpp
    GameViewport.h  GameViewport.cpp
    QmlUIBridge.h   QmlUIBridge.cpp
    renderer/GLBackend.cpp
)

qt_add_executable(aoqml ${QT_APP_SOURCES})

set_property(TARGET aoqml PROPERTY CXX_STANDARD 20)
set_property(TARGET aoqml PROPERTY CXX_STANDARD_REQUIRED ON)
set_property(TARGET aoqml PROPERTY AUTOMOC ON)

target_include_directories(aoqml PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

# QML module — registers QML_ELEMENT types (GameViewport, QmlUIBridge) and
# bundles the QML files as Qt resources under qrc:/AO/.
qt_add_qml_module(aoqml
    URI AO
    VERSION 1.0
    RESOURCE_PREFIX /
    NO_RESOURCE_TARGET_PATH
    QML_FILES
        qml/main.qml
        qml/screens/ServerList.qml
        qml/screens/CharSelect.qml
        qml/screens/Courtroom.qml
)

target_link_libraries(aoqml PRIVATE
    ao_protocol
    Qt::Core
    Qt::Gui
    Qt::Qml
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

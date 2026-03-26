find_package(Qt6 6.8 REQUIRED COMPONENTS Core Gui Quick Qml QuickControls2)
qt_standard_project_setup(REQUIRES 6.8)

# Sources are added to this list across commits. The QML module handles
# SceneTextureItem separately (it carries QML_ELEMENT).
set(QT_APP_SOURCES
    main.cpp
    EngineEventBridge.cpp
    QtGameWindow.cpp
    render/RenderBridge.cpp
    ui/QtUIRenderer.cpp
    ui/models/ServerListModel.cpp
    ui/models/ChatModel.cpp
    ui/models/PlayerListModel.cpp
    ui/models/EvidenceModel.cpp
    ui/models/MusicAreaModel.cpp
    ui/controllers/ServerListController.cpp
    ui/controllers/CharSelectController.cpp
    ui/controllers/CourtroomController.cpp
)

if(APPLE)
    qt_add_executable(aoqt MACOSX_BUNDLE ${QT_APP_SOURCES})
elseif(WIN32)
    qt_add_executable(aoqt WIN32 ${QT_APP_SOURCES})
else()
    qt_add_executable(aoqt ${QT_APP_SOURCES})
endif()

# SceneTextureItem carries QML_ELEMENT so it must be listed as a SOURCES
# entry of the QML module — Qt's moc scanner picks it up there.
qt_add_qml_module(aoqt
    URI "AO"
    VERSION 1.0
    QML_FILES
        qml/Main.qml
        qml/screens/ServerListScreen.qml
        qml/screens/CharSelectScreen.qml
        qml/screens/CourtroomScreen.qml
        qml/components/ChatPanel.qml
        qml/components/ICLogPanel.qml
        qml/components/ICChatInput.qml
        qml/components/EvidencePanel.qml
        qml/components/EmoteSelector.qml
        qml/components/HealthBars.qml
        qml/components/MusicArea.qml
        qml/components/PlayerList.qml
        qml/components/SideSelect.qml
        qml/components/TimerDisplay.qml
        qml/components/InterjectionOverlay.qml
        qml/components/MessageOptions.qml
        qml/components/DisconnectModal.qml
        qml/components/DebugOverlay.qml
    SOURCES
        render/SceneTextureItem.h
        render/SceneTextureItem.cpp
)

# Metal-specific code requires Objective-C++ and must be in separate .mm files.
if(APPLE)
    target_sources(aoqt PRIVATE
        render/SceneTextureItem_apple.mm
        QtGameWindow_apple.mm
    )
endif()

set_property(TARGET aoqt PROPERTY CXX_STANDARD 20)
# render/ is added so Qt's generated qmltyperegistrations.cpp can resolve
# #include "SceneTextureItem.h" (codegen drops the subdirectory prefix).
target_include_directories(aoqt PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/render
)

target_link_libraries(aoqt PRIVATE
    ao_protocol
    Qt6::Core
    Qt6::Gui
    Qt6::Quick
    Qt6::Qml
    Qt6::QuickControls2
)

if(APPLE)
    target_link_libraries(aoqt PRIVATE aorender_metal
        "-framework Metal"
        "-framework MetalKit"
        "-framework QuartzCore"
    )
else()
    target_link_libraries(aoqt PRIVATE aorender_gl)
endif()

# Prefer the discrete GPU on Windows laptops with hybrid graphics.
# Without these exports the OpenGL ICD may not load at all on Optimus/APU systems.
if(WIN32)
    target_sources(aoqt PRIVATE gpu_preference.cpp)
endif()

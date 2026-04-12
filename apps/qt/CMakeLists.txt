qt_standard_project_setup(REQUIRES 6.11)

find_package(Qt6 6.11 REQUIRED COMPONENTS Core Gui GuiPrivate Quick Qml QuickControls2)
set(QT_NO_PRIVATE_MODULE_WARNING ON)

# Common output root for all AO.* QML modules — Qt Creator uses this to
# discover sub-modules for code completion and navigation.
set(QT_QML_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/qml")

qt_add_executable(aoqt main.cpp)

# ── AO.Components — generic, reusable layout primitives (no controllers) ───
qt_add_library(ao_components STATIC)
qt_add_qml_module(ao_components
    URI "AO.Components"
    VERSION 1.0
    QML_FILES
        qml/components/DockPanel.qml
        qml/components/ExpandingBox.qml
        qml/components/OverlayPanel.qml
)

# ── AO.Panels — concrete UI panels (controller-aware or pure display) ─────
qt_add_library(ao_panels STATIC)
qt_add_qml_module(ao_panels
    URI "AO.Panels"
    VERSION 1.0
    DEPENDENCIES
        AO.Components
    QML_FILES
        # HUD / overlay panels
        qml/panels/HealthBarDisplay.qml
        qml/panels/TimerDisplay.qml
        qml/panels/InterjectionOverlay.qml
        qml/panels/HealthPanel.qml

        # Courtroom panels
        qml/panels/OOCChatPanel.qml
        qml/panels/ICInputPanel.qml
        qml/panels/ICLogPanel.qml
        qml/panels/MusicAreaPanel.qml
        qml/panels/PlayerListPanel.qml
        qml/panels/EvidencePanel.qml
        qml/panels/EmoteSelectorPanel.qml
        qml/panels/MessageOptionsPanel.qml
        qml/panels/SideSelectPanel.qml
        qml/panels/DisconnectModal.qml
        qml/panels/DebugOverlay.qml
)

# ── AO — root module (entry point, screens, render integration) ────────────
# SceneTextureItem carries QML_ELEMENT so it must be listed as a SOURCES
# entry of the QML module — Qt's moc scanner picks it up there.
# GameViewport wraps SceneTextureItem and therefore stays in the root module
# to avoid leaking render-pipeline dependencies into a pure-QML static lib.
qt_add_qml_module(aoqt
    URI "AO"
    VERSION 1.0
    DEPENDENCIES
        AO.Components
        AO.Panels
    QML_FILES
        qml/Main.qml

        # GameViewport wraps the C++ SceneTextureItem — keep in root module.
        qml/panels/GameViewport.qml

        # Screens compose panels + components into full interactive views.
        qml/screens/CourtroomScreen.qml
        qml/screens/ServerListScreen.qml
        qml/screens/CharSelectScreen.qml
    SOURCES
        render/SceneTextureItem.h
        render/SceneTextureItem.cpp
)

set_property(TARGET aoqt PROPERTY CXX_STANDARD 20)

# render/ is added so Qt's generated qmltyperegistrations.cpp can resolve
# #include "SceneTextureItem.h" (codegen drops the subdirectory prefix).
# ui/controllers/ gives moc the controller headers for Q_OBJECT processing.
target_include_directories(aoqt PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/render
    ${CMAKE_CURRENT_SOURCE_DIR}/ui
    ${CMAKE_CURRENT_SOURCE_DIR}/ui/controllers
    ${CMAKE_CURRENT_SOURCE_DIR}/ui/models
)

# Non-QML sources: bridge, controllers, EngineEventBridge.
target_sources(aoqt PRIVATE
    render/IQtRenderBackend.h
    render/RenderBridge.h
    render/RenderBridge.cpp

    EngineInterface.h
    EngineInterface.cpp

    EngineEventBridge.h
    EngineEventBridge.cpp

    QtAppInterface.h
    QtAppInterface.cpp

    ui/CharIconProvider.h
    ui/CharIconProvider.cpp
    ui/EmoteIconProvider.h
    ui/EmoteIconProvider.cpp
    ui/QtImageWatcher.h
    ui/QtImageWatcher.cpp

    ui/controllers/IQtScreenController.h
    ui/controllers/ServerListController.h
    ui/controllers/ServerListController.cpp
    ui/controllers/CharSelectController.h
    ui/controllers/CharSelectController.cpp
    ui/controllers/AudioController.h
    ui/controllers/AudioController.cpp
    ui/controllers/ChatController.h
    ui/controllers/ChatController.cpp
    ui/controllers/ICController.h
    ui/controllers/ICController.cpp
    ui/controllers/PlayerController.h
    ui/controllers/PlayerController.cpp
    ui/controllers/EvidenceController.h
    ui/controllers/EvidenceController.cpp
    ui/controllers/MusicAreaController.h
    ui/controllers/MusicAreaController.cpp
    ui/controllers/HUDController.h
    ui/controllers/HUDController.cpp
    ui/controllers/DebugController.h
    ui/controllers/DebugController.cpp

    ui/models/CharListModel.h
    ui/models/CharListModel.cpp
    ui/models/ServerListModel.h
    ui/models/ServerListModel.cpp
    ui/models/ChatModel.h
    ui/models/ChatModel.cpp
    ui/models/EvidenceModel.h
    ui/models/EvidenceModel.cpp
    ui/models/MusicAreaModel.h
    ui/models/MusicAreaModel.cpp
    ui/models/PlayerListModel.h
    ui/models/PlayerListModel.cpp
    ui/models/EmoteModel.h
    ui/models/EmoteModel.cpp
    ui/models/ICLogModel.h
    ui/models/ICLogModel.cpp
)

target_link_libraries(aoqt PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::Quick
    Qt6::Qml
    Qt6::QuickControls2
    Qt6::GuiPrivate
    # Sub-module plugins (static registration into the executable)
    ao_componentsplugin
    ao_panelsplugin
    # ao_protocol provides ServerListScreen, ao::create_protocol(), ao::create_presenter(),
    # and transitively pulls in ao_net and ao_game.
    ao_protocol
)

# Platform GPU backend (IQtRenderBackend) and renderer plugin, both selected
# at link time.  The backend adapts Qt render-thread integration (context
# readiness, state restoration, texture format); the renderer plugin provides
# create_renderer() and transitively pulls in aoengine.
if(APPLE)
    target_sources(aoqt PRIVATE render/MetalRenderBackend.mm)
    target_link_libraries(aoqt PRIVATE aorender_metal
        "-framework Metal"
        "-framework MetalKit"
        "-framework QuartzCore"
    )
else()
    target_sources(aoqt PRIVATE render/GLRenderBackend.cpp)
    target_link_libraries(aoqt PRIVATE aorender_gl OpenGL::GL)
endif()

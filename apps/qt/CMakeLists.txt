find_package(Qt6 6.8 REQUIRED COMPONENTS Core OpenGL Gui Quick Qml QuickControls2)
qt_standard_project_setup(REQUIRES 6.8)

qt_add_executable(aoqt main.cpp)

# SceneTextureItem carries QML_ELEMENT so it must be listed as a SOURCES
# entry of the QML module — Qt's moc scanner picks it up there.
qt_add_qml_module(aoqt
    URI "AO"
    VERSION 1.0
    QML_FILES
        qml/Main.qml
        qml/ServerListScreen.qml
        qml/CharSelectScreen.qml
        qml/CourtroomScreen.qml
        qml/DummyScreen.qml
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
    render/RenderBridge.h
    render/RenderBridge.cpp

    EngineEventBridge.h
    EngineEventBridge.cpp

    AppContext.h
    AppContext.cpp

    ui/CharIconProvider.h
    ui/CharIconProvider.cpp

    ui/controllers/IQtScreenController.h
    ui/controllers/ServerListController.h
    ui/controllers/ServerListController.cpp
    ui/controllers/CharSelectController.h
    ui/controllers/CharSelectController.cpp
    ui/controllers/CourtroomController.h
    ui/controllers/CourtroomController.cpp
    ui/controllers/DummyController.h
    ui/controllers/DummyController.cpp

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
)

target_link_libraries(aoqt PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::Quick
    Qt6::Qml
    Qt6::QuickControls2
    Qt6::GuiPrivate
    # aorender_gl provides create_renderer() and transitively pulls in
    # aoengine (StateBuffer, RenderManager, GameThread, MediaManager, etc.)
    # and the GLEW + OpenGL link dependencies.
    aorender_gl
    # ao_protocol provides ServerListScreen, ao::create_protocol(), ao::create_presenter(),
    # and transitively pulls in ao_net and ao_game.
    ao_protocol
)

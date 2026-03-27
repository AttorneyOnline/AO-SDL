find_package(Qt6 6.8 REQUIRED COMPONENTS Core Gui Quick Qml QuickControls2)
qt_standard_project_setup(REQUIRES 6.8)

qt_add_executable(aoqt main.cpp)

# SceneTextureItem carries QML_ELEMENT so it must be listed as a SOURCES
# entry of the QML module — Qt's moc scanner picks it up there.
qt_add_qml_module(aoqt
    URI "AO"
    VERSION 1.0
    QML_FILES
        qml/Main.qml
    SOURCES
        render/SceneTextureItem.h
        render/SceneTextureItem.cpp
)

set_property(TARGET aoqt PROPERTY CXX_STANDARD 20)
# render/ is added so Qt's generated qmltyperegistrations.cpp can resolve
# #include "SceneTextureItem.h" (codegen drops the subdirectory prefix).
target_include_directories(aoqt PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/render
)

target_link_libraries(aoqt PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::Quick
    Qt6::Qml
    Qt6::QuickControls2
    Qt6::GuiPrivate
)

# Prefer the discrete GPU on Windows laptops with hybrid graphics.
if(WIN32)
    target_sources(aoqt PRIVATE gpu_preference.cpp)
endif()

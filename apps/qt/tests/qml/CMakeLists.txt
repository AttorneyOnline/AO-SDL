find_package(Qt6 REQUIRED COMPONENTS Test Quick QuickTest Qml QuickControls2)

# tst_qml_components — QtQuickTest runner for AO.Components QML files.
#
# QUICK_TEST_SOURCE_DIR  tells the runner where to find tst_*.qml files at
#                        runtime (the current source directory).
# QML_IMPORT_PATH        is forwarded to the QQmlEngine so it can resolve
#                        "import AO.Components" against the build output tree.
qt_add_executable(tst_qml_components main.cpp)
set_property(TARGET tst_qml_components PROPERTY CXX_STANDARD 20)

target_compile_definitions(tst_qml_components PRIVATE
    QUICK_TEST_SOURCE_DIR="${CMAKE_CURRENT_SOURCE_DIR}"
    QML_IMPORT_PATH="${QT_QML_OUTPUT_DIRECTORY}"
)

target_link_libraries(tst_qml_components PRIVATE
    Qt6::Test
    Qt6::Quick
    Qt6::QuickTest
    Qt6::Qml
    Qt6::QuickControls2
    ao_componentsplugin
)

add_test(NAME tst_qml_components COMMAND tst_qml_components)
set_tests_properties(tst_qml_components PROPERTIES LABELS "qt_frontend")

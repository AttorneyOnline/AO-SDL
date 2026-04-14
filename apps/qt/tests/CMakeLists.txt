find_package(Qt6 REQUIRED COMPONENTS Test)

# Helper: create one QTest executable and register it with CTest.
# Usage: ao_qt_test(<name> <source.cpp>)
function(ao_qt_test NAME SOURCE)
    qt_add_executable(${NAME} ${SOURCE})
    set_property(TARGET ${NAME} PROPERTY CXX_STANDARD 20)
    target_link_libraries(${NAME} PRIVATE
        Qt6::Test
        ao_qt_models
    )
    add_test(NAME ${NAME} COMMAND ${NAME})
    set_tests_properties(${NAME} PROPERTIES LABELS "qt_frontend")
endfunction()

ao_qt_test(tst_server_list_model  tst_server_list_model.cpp)
ao_qt_test(tst_char_list_model    tst_char_list_model.cpp)
ao_qt_test(tst_chat_model         tst_chat_model.cpp)
ao_qt_test(tst_iclog_model        tst_iclog_model.cpp)
ao_qt_test(tst_player_list_model  tst_player_list_model.cpp)
ao_qt_test(tst_evidence_model     tst_evidence_model.cpp)
ao_qt_test(tst_music_area_model   tst_music_area_model.cpp)
ao_qt_test(tst_emote_model        tst_emote_model.cpp)

# ── QML component tests ───────────────────────────────────────────────────────
add_subdirectory(qml)

# ── Umbrella target: build every Qt test binary in one --target invocation ────
add_custom_target(ao_qt_tests_all)
add_dependencies(ao_qt_tests_all
    tst_server_list_model
    tst_char_list_model
    tst_chat_model
    tst_iclog_model
    tst_player_list_model
    tst_evidence_model
    tst_music_area_model
    tst_emote_model
    tst_qml_components
)

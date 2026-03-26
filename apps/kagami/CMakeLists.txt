add_executable(kagami
    main.cpp
    ServerSettings.cpp
    TerminalUI.cpp
)
set_property(TARGET kagami PROPERTY CXX_STANDARD 20)
target_include_directories(kagami PRIVATE ${CMAKE_SOURCE_DIR}/engine)
target_link_libraries(kagami PRIVATE aoengine)

add_executable(kagami
    main.cpp
    ServerSettings.cpp
    TerminalUI.cpp
    LogSinkSetup.cpp
    WsWorkerPool.cpp
    MetricsCollector.cpp
    ReplCommandFactory.cpp
    commands/StopCommand.cpp
    commands/HelpCommand.cpp
    commands/SessionsCommand.cpp
    commands/StatusCommand.cpp
)
set_property(TARGET kagami PROPERTY CXX_STANDARD 20)
target_include_directories(kagami PRIVATE
    ${CMAKE_SOURCE_DIR}/engine
    ${CMAKE_CURRENT_SOURCE_DIR}
)
target_link_libraries(kagami PRIVATE kagami_server)
aosdl_strip(kagami)

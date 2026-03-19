include(FetchContent)

FetchContent_Declare(
    googletest
    URL https://github.com/google/googletest/archive/refs/tags/v1.14.0.zip
)
# On Windows, prevent overriding the parent project's compiler/linker settings.
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

set(TEST_SOURCES
    net/test_WebSocket.cpp
    ao/net/test_AOPacket.cpp
    ao/net/test_PacketTypes.cpp
    asset/test_AssetCache.cpp
    event/test_EventChannel.cpp
    game/test_ServerList.cpp
    utils/test_Base64.cpp
    utils/test_JsonValidation.cpp

    # Implementation files — no GL/SDL dependencies
    ${CMAKE_SOURCE_DIR}/src/utils/Base64.cpp
    ${CMAKE_SOURCE_DIR}/src/utils/JsonValidation.cpp
    ${CMAKE_SOURCE_DIR}/src/utils/Log.cpp
    ${CMAKE_SOURCE_DIR}/src/event/Event.cpp
    ${CMAKE_SOURCE_DIR}/src/event/ChatEvent.cpp
    ${CMAKE_SOURCE_DIR}/src/event/OutgoingChatEvent.cpp
    ${CMAKE_SOURCE_DIR}/src/event/UIEvent.cpp
    ${CMAKE_SOURCE_DIR}/src/event/ServerListEvent.cpp
    ${CMAKE_SOURCE_DIR}/src/event/CharacterListEvent.cpp
    ${CMAKE_SOURCE_DIR}/src/event/CharsCheckEvent.cpp
    ${CMAKE_SOURCE_DIR}/src/event/CharSelectRequestEvent.cpp
    ${CMAKE_SOURCE_DIR}/src/event/ServerConnectEvent.cpp
    ${CMAKE_SOURCE_DIR}/src/game/ServerList.cpp
    ${CMAKE_SOURCE_DIR}/src/asset/Asset.cpp
    ${CMAKE_SOURCE_DIR}/src/asset/AssetCache.cpp
    ${CMAKE_SOURCE_DIR}/src/ao/net/AOPacket.cpp
    ${CMAKE_SOURCE_DIR}/src/ao/net/PacketTypes.cpp
    ${CMAKE_SOURCE_DIR}/src/ao/net/PacketBehavior.cpp
    ${CMAKE_SOURCE_DIR}/src/ao/net/AOClient.cpp
    ${CMAKE_SOURCE_DIR}/src/net/WebSocket.cpp
    ${CMAKE_SOURCE_DIR}/src/net/KissnetTcpSocket.cpp
)

add_executable(aosdl_tests ${TEST_SOURCES})
set_property(TARGET aosdl_tests PROPERTY CXX_STANDARD 20)

target_include_directories(aosdl_tests PRIVATE
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_SOURCE_DIR}/third-party/header-libs
    ${CMAKE_SOURCE_DIR}/third-party/kissnet
    ${CMAKE_SOURCE_DIR}/tests/net   # MockTcpSocket.h
)

target_link_libraries(aosdl_tests PRIVATE GTest::gtest_main)

include(GoogleTest)
gtest_discover_tests(aosdl_tests)

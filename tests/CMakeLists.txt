include(FetchContent)

FetchContent_Declare(
    googletest
    URL https://github.com/google/googletest/archive/refs/tags/v1.14.0.zip
)
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

set(TEST_SOURCES
    net/test_WebSocket.cpp
    ao/net/test_AOPacket.cpp
    ao/net/test_PacketTypes.cpp
    ao/net/test_NewPackets.cpp
    ao/event/test_ICMessageEvent.cpp
    ao/game/test_AOEmotePlayer.cpp
    ao/asset/test_AOAssetLibrary.cpp
    ao/game/test_AOTextBox.cpp
    ao/game/test_AOBackground.cpp
    asset/test_AssetCache.cpp
    asset/test_ApngDecoder.cpp
    event/test_EventChannel.cpp
    event/test_BackgroundEvent.cpp
    game/test_ServerList.cpp
    render/test_AnimationPlayer.cpp
    render/test_Layer.cpp
    render/test_TextRendererLayout.cpp
    utils/test_Base64.cpp
    utils/test_BlendOps.cpp
    utils/test_JsonValidation.cpp
    utils/test_UTF8.cpp
)

add_executable(aosdl_tests ${TEST_SOURCES})
set_property(TARGET aosdl_tests PROPERTY CXX_STANDARD 20)

target_include_directories(aosdl_tests PRIVATE
    ${CMAKE_SOURCE_DIR}/tests/net   # MockTcpSocket.h
    ${CMAKE_SOURCE_DIR}/engine      # Access to engine internals (WebSocket.h, etc.)
)

target_link_libraries(aosdl_tests PRIVATE aoengine ao_protocol GTest::gtest_main)

include(GoogleTest)
gtest_discover_tests(aosdl_tests)

include(FetchContent)

FetchContent_Declare(
    googletest
    URL https://github.com/google/googletest/archive/refs/tags/v1.14.0.zip
)
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

set(TEST_SOURCES
    net/test_WebSocket.cpp
    net/test_HttpPool.cpp
    ao/net/test_AOPacket.cpp
    ao/net/test_PacketTypes.cpp
    ao/net/test_NewPackets.cpp
    ao/net/test_AOClient.cpp
    ao/event/test_ICMessageEvent.cpp
    ao/game/test_AOEmotePlayer.cpp
    ao/asset/test_AOAssetLibrary.cpp
    ao/asset/test_AOCharacterSheet.cpp
    ao/game/test_AOTextBox.cpp
    ao/game/test_AOBackground.cpp
    ao/game/test_ICMessageQueue.cpp
    ao/game/test_SceneEffects.cpp
    ao/game/test_SlideEffect.cpp
    ao/game/test_AOMessagePlayer.cpp
    ao/game/test_AOSceneCompositor.cpp
    ao/game/test_AOCourtroomPresenter.cpp
    asset/test_AssetLibrary.cpp
    asset/test_AssetCache.cpp
    asset/test_ApngDecoder.cpp
    asset/test_ImageDecoders.cpp
    asset/test_MountDirectory.cpp
    asset/test_MountHttp.cpp
    asset/test_MountManager.cpp
    event/test_EventChannel.cpp
    event/test_EventManager.cpp
    event/test_BackgroundEvent.cpp
    event/test_EventTypes.cpp
    game/test_ServerList.cpp
    game/test_Session.cpp
    game/test_TickProfiler.cpp
    render/test_AnimationPlayer.cpp
    render/test_Layer.cpp
    render/test_StateBuffer.cpp
    render/test_TextRendererLayout.cpp
    render/test_Transform.cpp
    render/test_TransformAnimator.cpp
    utils/test_Base64.cpp
    utils/test_BlendOps.cpp
    utils/test_JsonValidation.cpp
    utils/test_UTF8.cpp
    utils/test_Log.cpp
    net/test_NetworkThread.cpp
    ui/test_UIManager.cpp
    audio/test_AudioStream.cpp
    ao/ui/test_Screens.cpp
    render/test_TextMeshBuilder.cpp
    configuration/test_IConfiguration.cpp
    configuration/test_JsonConfiguration.cpp
    net/test_WebSocketCommon.cpp
    net/test_WebSocketServer.cpp
    net/test_NXMessage.cpp
    ao/net/test_AOServer.cpp
)

add_executable(aosdl_tests ${TEST_SOURCES}
    utils/test_StringHelpers.cpp)
set_property(TARGET aosdl_tests PROPERTY CXX_STANDARD 20)

target_include_directories(aosdl_tests PRIVATE
    ${CMAKE_SOURCE_DIR}/tests/net   # MockTcpSocket.h
    ${CMAKE_SOURCE_DIR}/engine      # Access to engine internals (WebSocket.h, etc.)
)

target_compile_definitions(aosdl_tests PRIVATE
    AO_TEST_FONT_PATH="${CMAKE_SOURCE_DIR}/third-party/imgui/misc/fonts/ProggyClean.ttf"
)

# Stub Texture2D so tests don't need the GL renderer plugin
target_sources(aosdl_tests PRIVATE stubs/StubTexture.cpp)

target_link_libraries(aosdl_tests PRIVATE ao_protocol nx_net GTest::gtest_main)

include(GoogleTest)
gtest_discover_tests(aosdl_tests)

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
    game/test_ASNReputationManager.cpp
    game/test_FirewallManager.cpp
    game/test_IPReputationService.cpp
    game/test_ServerList.cpp
    game/test_Session.cpp
    game/test_SpamDetector.cpp
    moderation/test_UnicodeClassifier.cpp
    moderation/test_UrlExtractor.cpp
    moderation/test_SlurFilter.cpp
    moderation/test_SafeHintLayer.cpp
    moderation/test_ModerationHeat.cpp
    moderation/test_ContentModerator.cpp
    moderation/test_RemoteClassifier.cpp
    moderation/test_RemoteDedupCache.cpp
    moderation/test_SemanticClusterer.cpp
    game/test_TickProfiler.cpp
    game/test_ACLFlags.cpp
    game/test_BanDuration.cpp
    game/test_DatabaseManager.cpp
    game/test_GameThread.cpp
    render/test_AnimationPlayer.cpp
    render/test_Layer.cpp
    render/test_StateBuffer.cpp
    render/test_TextRendererLayout.cpp
    render/test_Transform.cpp
    render/test_TransformAnimator.cpp
    utils/test_Base64.cpp
    utils/test_BlendOps.cpp
    utils/test_JsonValidation.cpp
    utils/test_JsonSchema.cpp
    utils/test_UTF8.cpp
    utils/test_Crypto.cpp
    utils/test_ImdsCredentialProvider.cpp
    utils/test_PBKDF2.cpp
    utils/test_Log.cpp
    utils/test_Metrics.cpp
    utils/test_PersistentMap.cpp
    net/test_WSClientThread.cpp
    net/test_HttpClient.cpp
    net/test_HttpServer.cpp
    net/test_HttpCoverage.cpp
    net/test_SSE.cpp
    net/test_PlatformSocket.cpp
    platform/test_Socket.cpp
    platform/test_Poll.cpp
    ui/test_UIManager.cpp
    audio/test_AudioStream.cpp
    ao/ui/test_Screens.cpp
    render/test_TextMeshBuilder.cpp
    render/test_MeshAsset.cpp
    configuration/test_IConfiguration.cpp
    configuration/test_JsonConfiguration.cpp
    configuration/test_ServerSettings.cpp
    net/test_WebSocketCommon.cpp
    net/test_WebSocketServer.cpp
    net/test_NXMessage.cpp
    net/test_NXBroadcast.cpp
    net/test_ProxyProtocol.cpp
    net/test_RateLimiter.cpp
    net/test_TrustedProxyList.cpp
    net/test_RestRouter.cpp
    net/test_RFC9112.cpp
    net/test_RFC9110.cpp
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

# -- Schema generator tests (generate from test YAML, compile, assert) --------
if(AOSDL_SCHEMAS_AVAILABLE)
    set(_TEST_SCHEMA_YAML "${CMAKE_CURRENT_SOURCE_DIR}/schemas/test_openapi.yaml")
    set(_TEST_SCHEMA_CPP  "${CMAKE_CURRENT_BINARY_DIR}/generated/TestSchemas.cpp")

    add_custom_command(
        OUTPUT  "${_TEST_SCHEMA_CPP}"
        COMMAND ${Python3_EXECUTABLE}
                "${CMAKE_SOURCE_DIR}/scripts/generate_schemas.py"
                "${_TEST_SCHEMA_YAML}"
                "${_TEST_SCHEMA_CPP}"
                --prefix test_schema
        DEPENDS "${_TEST_SCHEMA_YAML}"
                "${CMAKE_SOURCE_DIR}/scripts/generate_schemas.py"
        COMMENT "Generating test schemas from test_openapi.yaml"
    )
    target_sources(aosdl_tests PRIVATE
        "${_TEST_SCHEMA_CPP}"
        schemas/test_SchemaGenerator.cpp
    )
    target_compile_definitions(aosdl_tests PRIVATE AOSDL_HAS_TEST_SCHEMAS=1)
endif()

# =============================================================================
# SDL widget tests — headless ImGui via NullGPUBackend
# =============================================================================

# imgui_test: core ImGui + null backend (no SDL, no GL/Metal)
set(IMGUI_DIR ${CMAKE_SOURCE_DIR}/third-party/imgui)
add_library(imgui_test STATIC
    ${IMGUI_DIR}/imgui.cpp
    ${IMGUI_DIR}/imgui_draw.cpp
    ${IMGUI_DIR}/imgui_tables.cpp
    ${IMGUI_DIR}/imgui_widgets.cpp
    ${IMGUI_DIR}/backends/imgui_impl_null.cpp
)
target_compile_definitions(imgui_test PUBLIC IMGUI_USE_WCHAR32)
target_include_directories(imgui_test PUBLIC
    ${IMGUI_DIR}
    ${IMGUI_DIR}/backends
)
set_property(TARGET imgui_test PROPERTY CXX_STANDARD 20)

# Widget source files from the SDL app (compiled into the test target)
set(SDL_WIDGET_SOURCES
    ${CMAKE_SOURCE_DIR}/apps/sdl/ui/widgets/ChatWidget.cpp
    ${CMAKE_SOURCE_DIR}/apps/sdl/ui/widgets/HealthBarWidget.cpp
    ${CMAKE_SOURCE_DIR}/apps/sdl/ui/widgets/TimerWidget.cpp
    ${CMAKE_SOURCE_DIR}/apps/sdl/ui/widgets/PlayerListWidget.cpp
    ${CMAKE_SOURCE_DIR}/apps/sdl/ui/widgets/EvidenceWidget.cpp
    ${CMAKE_SOURCE_DIR}/apps/sdl/ui/widgets/DisconnectModalWidget.cpp
    ${CMAKE_SOURCE_DIR}/apps/sdl/ui/widgets/MusicAreaWidget.cpp
)

set(SDL_WIDGET_TEST_SOURCES
    sdl/test_ChatWidget.cpp
    sdl/test_HealthBarWidget.cpp
    sdl/test_TimerWidget.cpp
    sdl/test_PlayerListWidget.cpp
    sdl/test_EvidenceWidget.cpp
    sdl/test_DisconnectModalWidget.cpp
    sdl/test_MusicAreaWidget.cpp
    sdl/test_CourtroomState.cpp
)

add_executable(aosdl_widget_tests ${SDL_WIDGET_TEST_SOURCES} ${SDL_WIDGET_SOURCES})
set_property(TARGET aosdl_widget_tests PROPERTY CXX_STANDARD 20)
target_include_directories(aosdl_widget_tests PRIVATE
    ${CMAKE_SOURCE_DIR}/apps/sdl        # ui/widgets/*.h, render/IGPUBackend.h
    ${CMAKE_SOURCE_DIR}/tests/sdl       # NullGPUBackend.h, ImGuiTestFixture.h
    ${CMAKE_SOURCE_DIR}/engine          # engine internals
)
target_sources(aosdl_widget_tests PRIVATE stubs/StubTexture.cpp)
target_link_libraries(aosdl_widget_tests PRIVATE
    ao_protocol
    imgui_test
    GTest::gtest_main
)

include(GoogleTest)
gtest_discover_tests(aosdl_tests)
gtest_discover_tests(aosdl_widget_tests)

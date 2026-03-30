#include "ImGuiTestFixture.h"

#include "ui/widgets/ICMessageState.h"
#include "ui/widgets/MusicAreaWidget.h"

#include "event/AreaUpdateEvent.h"
#include "event/EventManager.h"
#include "event/MusicListEvent.h"
#include "event/NowPlayingEvent.h"

class MusicAreaWidgetTest : public ImGuiTestFixture {
  protected:
    void SetUp() override {
        ImGuiTestFixture::SetUp();
        drain<MusicListEvent>();
        drain<AreaUpdateEvent>();
        drain<NowPlayingEvent>();
    }

    ICMessageState ic_state_;
    MusicAreaWidget widget_{&ic_state_};
};

TEST_F(MusicAreaWidgetTest, HandleEvents_FullMusicList) {
    std::vector<std::string> areas = {"Lobby", "Courtroom"};
    std::vector<std::string> tracks = {"Ace Attorney", "AA/Logic/Logic & Trick.opus", "AA/Trial.mp3"};
    EventManager::instance().get_channel<MusicListEvent>().publish(MusicListEvent(areas, tracks));
    widget_.handle_events();

    auto& cs = CourtroomState::instance();
    ASSERT_EQ(cs.areas.size(), 2);
    EXPECT_EQ(cs.areas[0], "Lobby");
    ASSERT_EQ(cs.tracks.size(), 3);
    EXPECT_EQ(cs.tracks[0], "Ace Attorney");

    // Area metadata vectors should be initialized
    EXPECT_EQ(cs.area_players.size(), 2);
    EXPECT_EQ(cs.area_status.size(), 2);
}

TEST_F(MusicAreaWidgetTest, HandleEvents_PartialTracksOnly) {
    auto& ch = EventManager::instance().get_channel<MusicListEvent>();

    // Full list first
    ch.publish(MusicListEvent({"Lobby"}, {"Track1"}, false));
    widget_.handle_events();

    // Partial with only tracks
    ch.publish(MusicListEvent({}, {"NewTrack1", "NewTrack2"}, true));
    widget_.handle_events();

    auto& cs = CourtroomState::instance();
    EXPECT_EQ(cs.areas.size(), 1); // areas unchanged
    EXPECT_EQ(cs.areas[0], "Lobby");
    EXPECT_EQ(cs.tracks.size(), 2); // tracks replaced
    EXPECT_EQ(cs.tracks[0], "NewTrack1");
}

TEST_F(MusicAreaWidgetTest, HandleEvents_PartialAreasOnly) {
    auto& ch = EventManager::instance().get_channel<MusicListEvent>();

    ch.publish(MusicListEvent({"Lobby"}, {"Track1"}, false));
    widget_.handle_events();

    ch.publish(MusicListEvent({"Lobby", "Courtroom"}, {}, true));
    widget_.handle_events();

    auto& cs = CourtroomState::instance();
    EXPECT_EQ(cs.areas.size(), 2);  // areas replaced
    EXPECT_EQ(cs.tracks.size(), 1); // tracks unchanged
}

TEST_F(MusicAreaWidgetTest, HandleEvents_AreaUpdatePlayers) {
    auto& ch = EventManager::instance().get_channel<MusicListEvent>();
    ch.publish(MusicListEvent({"Lobby", "Courtroom"}, {}));
    widget_.handle_events();

    EventManager::instance().get_channel<AreaUpdateEvent>().publish(
        AreaUpdateEvent(AreaUpdateEvent::PLAYERS, {"5", "3"}));
    widget_.handle_events();

    auto& cs = CourtroomState::instance();
    EXPECT_EQ(cs.area_players[0], 5);
    EXPECT_EQ(cs.area_players[1], 3);
}

TEST_F(MusicAreaWidgetTest, HandleEvents_AreaUpdateStatus) {
    auto& ch = EventManager::instance().get_channel<MusicListEvent>();
    ch.publish(MusicListEvent({"Lobby", "Courtroom"}, {}));
    widget_.handle_events();

    EventManager::instance().get_channel<AreaUpdateEvent>().publish(
        AreaUpdateEvent(AreaUpdateEvent::STATUS, {"CASING", "RECESS"}));
    widget_.handle_events();

    auto& cs = CourtroomState::instance();
    EXPECT_EQ(cs.area_status[0], "CASING");
    EXPECT_EQ(cs.area_status[1], "RECESS");
}

TEST_F(MusicAreaWidgetTest, HandleEvents_AreaUpdateLock) {
    auto& ch = EventManager::instance().get_channel<MusicListEvent>();
    ch.publish(MusicListEvent({"Lobby"}, {}));
    widget_.handle_events();

    EventManager::instance().get_channel<AreaUpdateEvent>().publish(AreaUpdateEvent(AreaUpdateEvent::LOCK, {"LOCKED"}));
    widget_.handle_events();

    EXPECT_EQ(CourtroomState::instance().area_lock[0], "LOCKED");
}

TEST_F(MusicAreaWidgetTest, HandleEvents_NowPlayingTrimsSongName) {
    EventManager::instance().get_channel<NowPlayingEvent>().publish(NowPlayingEvent("AA/Logic/Logic & Trick.opus"));
    widget_.handle_events();

    EXPECT_EQ(CourtroomState::instance().now_playing, "Logic & Trick");
}

TEST_F(MusicAreaWidgetTest, HandleEvents_NowPlayingPlainName) {
    EventManager::instance().get_channel<NowPlayingEvent>().publish(NowPlayingEvent("Track Name"));
    widget_.handle_events();

    EXPECT_EQ(CourtroomState::instance().now_playing, "Track Name");
}

TEST_F(MusicAreaWidgetTest, RenderSmokeTest_Empty) {
    with_frame([&] { widget_.render(); });
}

TEST_F(MusicAreaWidgetTest, RenderSmokeTest_WithTracks) {
    EventManager::instance().get_channel<MusicListEvent>().publish(
        MusicListEvent({"Lobby"}, {"Ace Attorney", "AA/Logic/Logic & Trick.opus", "AA/Trial.mp3"}));
    widget_.handle_events();
    with_frame([&] { widget_.render(); });
}

// ===========================================================================
// Issue #85: SIGSEGV when render() is called after widget recreation while
// CourtroomState.tracks retains stale data from a previous session.
// ===========================================================================

TEST_F(MusicAreaWidgetTest, Render_StaleSingletonState_DoesNotCrash) {
    // Simulate a previous session that populated CourtroomState.
    auto& cs = CourtroomState::instance();
    cs.tracks = {"Category", "song1.opus", "song2.opus", "song3.mp3"};
    cs.areas = {"Lobby"};
    cs.area_players = {5};
    cs.area_status = {"CASING"};
    cs.area_cm = {"FREE"};
    cs.area_lock = {"FREE"};

    // Create a NEW widget (simulating CourtroomController recreation after
    // a character change). Its local caches are empty, but cs.tracks has
    // 4 entries — the exact desync that caused the SIGSEGV.
    ICMessageState fresh_state;
    MusicAreaWidget fresh_widget(&fresh_state);

    // handle_events() with no pending events — caches stay empty.
    fresh_widget.handle_events();

    // render() must survive: the fix detects the size mismatch and calls
    // rebuild_track_caches() before accessing the local vectors.
    with_frame([&] { EXPECT_NO_FATAL_FAILURE(fresh_widget.render()); });
}

TEST_F(MusicAreaWidgetTest, Render_SyncsTrackCachesFromSingleton) {
    auto& cs = CourtroomState::instance();
    cs.tracks = {"Jazz", "smooth.opus", "cool.mp3"};

    ICMessageState fresh_state;
    MusicAreaWidget fresh_widget(&fresh_state);

    // First render syncs caches; second render confirms stability.
    with_frame([&] { fresh_widget.render(); });
    with_frame([&] { fresh_widget.render(); });
}

TEST_F(MusicAreaWidgetTest, Render_MultipleWidgetRecreations) {
    EventManager::instance().get_channel<MusicListEvent>().publish(
        MusicListEvent({"Area"}, {"Cat", "a.opus", "b.opus"}));
    widget_.handle_events();
    with_frame([&] { widget_.render(); });

    // First recreation — no new events, stale singleton state.
    ICMessageState state2;
    MusicAreaWidget widget2(&state2);
    widget2.handle_events();
    with_frame([&] { EXPECT_NO_FATAL_FAILURE(widget2.render()); });

    // Second recreation.
    ICMessageState state3;
    MusicAreaWidget widget3(&state3);
    with_frame([&] { EXPECT_NO_FATAL_FAILURE(widget3.render()); });
}

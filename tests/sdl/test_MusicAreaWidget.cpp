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

#include <gtest/gtest.h>

#include "event/EventManager.h"
#include "game/GameAction.h"
#include "game/GameRoom.h"
#include "net/SSEEvent.h"
#include "net/nx/NXServer.h"

#include <json.hpp>

// ===========================================================================
// Test fixture — creates a GameRoom with an actual NXServer so that
// broadcast→SSE serialization uses the real code path.
// ===========================================================================

class NXBroadcastTest : public ::testing::Test {
  protected:
    void SetUp() override {
        // Drain any stale events from previous tests
        while (EventManager::instance().get_channel<SSEEvent>().get_event()) {
        }

        // Set up a minimal GameRoom
        room_.characters = {"Phoenix", "Edgeworth", "Maya"};
        room_.reset_taken();
        room_.areas = {"Courtroom 1", "Lobby"};
        room_.build_area_index();
        room_.build_char_id_index();

        // Construct NXServer — this registers all broadcast callbacks on room_
        nx_ = std::make_unique<NXServer>(room_);

        // Create a session for the sender
        auto session = room_.create_session(1, "aonx");
        session->area = "Courtroom 1";
        session->display_name = "TestPlayer";
        session->joined = true;
    }

    /// Drain the next SSEEvent from the channel, or return nullopt.
    std::optional<SSEEvent> drain_sse() {
        return EventManager::instance().get_channel<SSEEvent>().get_event();
    }

    GameRoom room_;
    std::unique_ptr<NXServer> nx_;
};

// ===========================================================================
// Tests
// ===========================================================================

TEST_F(NXBroadcastTest, IcMessagePublishesSSEEvent) {
    ICAction action;
    action.sender_id = 1;
    action.character = "Phoenix";
    action.message = "Objection!";
    action.showname = "Phoenix Wright";
    action.side = "def";
    action.char_id = 0;

    room_.handle_ic(action, "Courtroom 1");

    auto evt = drain_sse();
    ASSERT_TRUE(evt.has_value());
    EXPECT_EQ(evt->event, "ic_message");
    EXPECT_EQ(evt->area, "Courtroom 1");

    auto j = nlohmann::json::parse(evt->data);
    EXPECT_EQ(j["character"], "Phoenix");
    EXPECT_EQ(j["message"], "Objection!");
    EXPECT_EQ(j["showname"], "Phoenix Wright");
    EXPECT_EQ(j["side"], "def");
}

TEST_F(NXBroadcastTest, OocMessagePublishesSSEEvent) {
    OOCAction action;
    action.sender_id = 1;
    action.name = "TestPlayer";
    action.message = "Hello everyone!";

    room_.handle_ooc(action, "Courtroom 1");

    auto evt = drain_sse();
    ASSERT_TRUE(evt.has_value());
    EXPECT_EQ(evt->event, "ooc_message");
    EXPECT_EQ(evt->area, "Courtroom 1");

    auto j = nlohmann::json::parse(evt->data);
    EXPECT_EQ(j["name"], "TestPlayer");
    EXPECT_EQ(j["message"], "Hello everyone!");
}

TEST_F(NXBroadcastTest, CharSelectPublishesGlobalSSEEvent) {
    CharSelectAction action;
    action.sender_id = 1;
    action.character_id = 0; // Phoenix

    room_.handle_char_select(action);

    // char_select produces both a char_select event and a chars_taken event
    // Drain all SSE events
    std::vector<SSEEvent> events;
    while (auto evt = drain_sse())
        events.push_back(std::move(*evt));

    // Find the char_taken event with char_id and available fields
    bool found_select = false;
    for (auto& e : events) {
        if (e.event == "char_taken") {
            auto j = nlohmann::json::parse(e.data);
            if (j.contains("char_id")) {
                found_select = true;
                EXPECT_EQ(j["char_id"], "0");
                EXPECT_EQ(j["available"], false);
                EXPECT_TRUE(j.contains("user_id"));
                EXPECT_EQ(e.area, ""); // global broadcast
            }
        }
    }
    EXPECT_TRUE(found_select) << "Expected a char_taken event with char_id";
}

TEST_F(NXBroadcastTest, MusicChangePublishesSSEEvent) {
    MusicAction action;
    action.sender_id = 1;
    action.track = "trial.mp3";
    action.showname = "TestPlayer";
    action.channel = 0;
    action.looping = true;

    room_.handle_music(action);

    // Drain events — look for music_change
    std::vector<SSEEvent> events;
    while (auto evt = drain_sse())
        events.push_back(std::move(*evt));

    bool found_music = false;
    for (auto& e : events) {
        if (e.event == "music_change") {
            found_music = true;
            auto j = nlohmann::json::parse(e.data);
            EXPECT_EQ(j["name"], "trial.mp3");
            EXPECT_EQ(j["showname"], "TestPlayer");
            EXPECT_EQ(j["channel"], 0);
            EXPECT_EQ(j["looping"], true);
        }
    }
    EXPECT_TRUE(found_music) << "Expected a music_change SSE event";
}

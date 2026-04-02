#include <gtest/gtest.h>

#include "event/EventManager.h"
#include "game/GameAction.h"
#include "game/GameRoom.h"
#include "net/SSEEvent.h"

#include <json.hpp>

// ===========================================================================
// Test fixture — creates a GameRoom with NX broadcast callbacks wired
// via the same path as NXServer (registers broadcast callbacks that
// serialize events to JSON and publish SSEEvents).
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

        // Register broadcast callbacks that mirror NXServer's implementation
        room_.add_ic_broadcast([this](const std::string& area, const ICEvent& evt) {
            auto& a = evt.action;
            nlohmann::json j;
            j["character"] = a.character;
            j["message"] = a.message;
            j["showname"] = a.showname;
            j["side"] = a.side;
            j["char_id"] = a.char_id;
            SSEEvent sse;
            sse.event = "ic_message";
            sse.data = j.dump();
            sse.area = area;
            EventManager::instance().get_channel<SSEEvent>().publish(std::move(sse));
        });

        room_.add_ooc_broadcast([this](const std::string& area, const OOCEvent& evt) {
            SSEEvent sse;
            sse.event = "ooc_message";
            sse.data = nlohmann::json({{"name", evt.action.name}, {"message", evt.action.message}}).dump();
            sse.area = area;
            EventManager::instance().get_channel<SSEEvent>().publish(std::move(sse));
        });

        room_.add_char_select_broadcast([this](const CharSelectEvent& evt) {
            SSEEvent sse;
            sse.event = "char_taken";
            sse.data = nlohmann::json({{"client_id", evt.client_id},
                                       {"character_id", evt.character_id},
                                       {"character_name", evt.character_name}})
                           .dump();
            sse.area = ""; // global
            EventManager::instance().get_channel<SSEEvent>().publish(std::move(sse));
        });

        room_.add_chars_taken_broadcast([this](const std::vector<int>& taken) {
            SSEEvent sse;
            sse.event = "char_taken";
            sse.data = nlohmann::json({{"taken", taken}}).dump();
            sse.area = ""; // global
            EventManager::instance().get_channel<SSEEvent>().publish(std::move(sse));
        });

        room_.add_music_broadcast([this](const std::string& area, const MusicEvent& evt) {
            SSEEvent sse;
            sse.event = "music_change";
            sse.data = nlohmann::json({{"track", evt.action.track},
                                       {"showname", evt.action.showname},
                                       {"channel", evt.action.channel},
                                       {"looping", evt.action.looping}})
                           .dump();
            sse.area = area;
            EventManager::instance().get_channel<SSEEvent>().publish(std::move(sse));
        });

        // Create a session for the sender
        auto& session = room_.create_session(1, "aonx");
        session.area = "Courtroom 1";
        session.display_name = "TestPlayer";
        session.joined = true;
    }

    /// Drain the next SSEEvent from the channel, or return nullopt.
    std::optional<SSEEvent> drain_sse() {
        return EventManager::instance().get_channel<SSEEvent>().get_event();
    }

    GameRoom room_;
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

    // Find the char_taken event with character_name
    bool found_select = false;
    for (auto& e : events) {
        if (e.event == "char_taken") {
            auto j = nlohmann::json::parse(e.data);
            if (j.contains("character_name")) {
                found_select = true;
                EXPECT_EQ(j["character_id"], 0);
                EXPECT_EQ(j["character_name"], "Phoenix");
                EXPECT_EQ(e.area, ""); // global broadcast
            }
        }
    }
    EXPECT_TRUE(found_select) << "Expected a char_taken event with character_name";
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
            EXPECT_EQ(j["track"], "trial.mp3");
            EXPECT_EQ(j["showname"], "TestPlayer");
            EXPECT_EQ(j["channel"], 0);
            EXPECT_EQ(j["looping"], true);
        }
    }
    EXPECT_TRUE(found_music) << "Expected a music_change SSE event";
}

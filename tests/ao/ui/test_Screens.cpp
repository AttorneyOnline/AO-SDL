#include "ao/ui/screens/CharSelectScreen.h"
#include "ao/ui/screens/CourtroomScreen.h"
#include "ao/ui/screens/ServerListScreen.h"

#include "event/EventManager.h"
#include "event/UIEvent.h"

#include <gtest/gtest.h>
#include <memory>
#include <string>

// ---------------------------------------------------------------------------
// Minimal ScreenController stub for enter()/exit() testing
// ---------------------------------------------------------------------------

class StubScreenController : public ScreenController {
  public:
    void push_screen(std::unique_ptr<Screen> /*screen*/) override {
    }
    void pop_screen() override {
    }
};

// ===========================================================================
// ServerListScreen
// ===========================================================================

// --- screen_id ---

TEST(ServerListScreen, ScreenIdReturnsServerList) {
    ServerListScreen screen;
    EXPECT_EQ(screen.screen_id(), "server_list");
}

TEST(ServerListScreen, StaticIdMatchesInstanceId) {
    ServerListScreen screen;
    EXPECT_EQ(ServerListScreen::ID, screen.screen_id());
}

// --- Initial state ---

TEST(ServerListScreen, InitialServersListIsEmpty) {
    ServerListScreen screen;
    EXPECT_TRUE(screen.get_servers().empty());
}

TEST(ServerListScreen, InitialSelectedIsNegativeOne) {
    ServerListScreen screen;
    EXPECT_EQ(screen.get_selected(), -1);
}

// --- enter / exit lifecycle ---

TEST(ServerListScreen, EnterDoesNotCrash) {
    ServerListScreen screen;
    StubScreenController ctrl;
    EXPECT_NO_FATAL_FAILURE(screen.enter(ctrl));
}

TEST(ServerListScreen, ExitDoesNotCrash) {
    ServerListScreen screen;
    StubScreenController ctrl;
    screen.enter(ctrl);
    EXPECT_NO_FATAL_FAILURE(screen.exit());
}

TEST(ServerListScreen, EnterThenExitLifecycle) {
    ServerListScreen screen;
    StubScreenController ctrl;
    screen.enter(ctrl);
    screen.exit();
    // After exit, screen should still report correct id and empty state
    EXPECT_EQ(screen.screen_id(), "server_list");
    EXPECT_TRUE(screen.get_servers().empty());
    EXPECT_EQ(screen.get_selected(), -1);
}

TEST(ServerListScreen, ReEnterAfterExit) {
    ServerListScreen screen;
    StubScreenController ctrl;
    screen.enter(ctrl);
    screen.exit();
    // Re-entering should not crash
    EXPECT_NO_FATAL_FAILURE(screen.enter(ctrl));
}

// --- handle_events with no pending events ---

TEST(ServerListScreen, HandleEventsWithNoEventsDoesNotCrash) {
    ServerListScreen screen;
    StubScreenController ctrl;
    screen.enter(ctrl);
    EXPECT_NO_FATAL_FAILURE(screen.handle_events());
}

TEST(ServerListScreen, HandleEventsMultipleTimesDoesNotCrash) {
    ServerListScreen screen;
    StubScreenController ctrl;
    screen.enter(ctrl);
    for (int i = 0; i < 5; ++i) {
        EXPECT_NO_FATAL_FAILURE(screen.handle_events());
    }
}

TEST(ServerListScreen, HandleEventsPreservesEmptyState) {
    ServerListScreen screen;
    StubScreenController ctrl;
    screen.enter(ctrl);
    screen.handle_events();
    EXPECT_TRUE(screen.get_servers().empty());
    EXPECT_EQ(screen.get_selected(), -1);
}

// --- select_server with empty list ---

TEST(ServerListScreen, SelectServerOnEmptyListIsNoOp) {
    ServerListScreen screen;
    StubScreenController ctrl;
    screen.enter(ctrl);
    screen.select_server(0);
    EXPECT_EQ(screen.get_selected(), -1);
}

TEST(ServerListScreen, SelectServerNegativeIndexIsNoOp) {
    ServerListScreen screen;
    StubScreenController ctrl;
    screen.enter(ctrl);
    screen.select_server(-1);
    EXPECT_EQ(screen.get_selected(), -1);
}

// ===========================================================================
// CharSelectScreen
// ===========================================================================

// --- screen_id ---

TEST(CharSelectScreen, ScreenIdReturnsCharSelect) {
    CharSelectScreen screen;
    EXPECT_EQ(screen.screen_id(), "char_select");
}

TEST(CharSelectScreen, StaticIdMatchesInstanceId) {
    CharSelectScreen screen;
    EXPECT_EQ(CharSelectScreen::ID, screen.screen_id());
}

// --- Initial state ---

TEST(CharSelectScreen, InitialCharsListIsEmpty) {
    CharSelectScreen screen;
    EXPECT_TRUE(screen.get_chars().empty());
}

TEST(CharSelectScreen, InitialSelectedIsNegativeOne) {
    CharSelectScreen screen;
    EXPECT_EQ(screen.get_selected(), -1);
}

// --- enter / exit lifecycle ---

TEST(CharSelectScreen, EnterDoesNotCrash) {
    CharSelectScreen screen;
    StubScreenController ctrl;
    EXPECT_NO_FATAL_FAILURE(screen.enter(ctrl));
}

TEST(CharSelectScreen, ExitDoesNotCrash) {
    CharSelectScreen screen;
    StubScreenController ctrl;
    screen.enter(ctrl);
    EXPECT_NO_FATAL_FAILURE(screen.exit());
}

TEST(CharSelectScreen, EnterThenExitLifecycle) {
    CharSelectScreen screen;
    StubScreenController ctrl;
    screen.enter(ctrl);
    screen.exit();
    // After exit, screen should still report correct id and empty state
    EXPECT_EQ(screen.screen_id(), "char_select");
    EXPECT_TRUE(screen.get_chars().empty());
    EXPECT_EQ(screen.get_selected(), -1);
}

TEST(CharSelectScreen, ReEnterAfterExit) {
    CharSelectScreen screen;
    StubScreenController ctrl;
    screen.enter(ctrl);
    screen.exit();
    EXPECT_NO_FATAL_FAILURE(screen.enter(ctrl));
}

// --- handle_events with no pending events ---

TEST(CharSelectScreen, HandleEventsWithNoEventsDoesNotCrash) {
    CharSelectScreen screen;
    StubScreenController ctrl;
    screen.enter(ctrl);
    EXPECT_NO_FATAL_FAILURE(screen.handle_events());
}

TEST(CharSelectScreen, HandleEventsMultipleTimesDoesNotCrash) {
    CharSelectScreen screen;
    StubScreenController ctrl;
    screen.enter(ctrl);
    for (int i = 0; i < 5; ++i) {
        EXPECT_NO_FATAL_FAILURE(screen.handle_events());
    }
}

TEST(CharSelectScreen, HandleEventsPreservesEmptyState) {
    CharSelectScreen screen;
    StubScreenController ctrl;
    screen.enter(ctrl);
    screen.handle_events();
    EXPECT_TRUE(screen.get_chars().empty());
    EXPECT_EQ(screen.get_selected(), -1);
}

// --- select_character with empty list ---

TEST(CharSelectScreen, SelectCharacterOnEmptyListIsNoOp) {
    CharSelectScreen screen;
    StubScreenController ctrl;
    screen.enter(ctrl);
    screen.select_character(0);
    EXPECT_EQ(screen.get_selected(), -1);
}

TEST(CharSelectScreen, SelectCharacterNegativeIndexIsNoOp) {
    CharSelectScreen screen;
    StubScreenController ctrl;
    screen.enter(ctrl);
    screen.select_character(-1);
    EXPECT_EQ(screen.get_selected(), -1);
}

// ===========================================================================
// CourtroomScreen
// ===========================================================================

// CourtroomScreen's constructor launches an async background thread that
// polls for character data with sleeps (up to 5 seconds). To avoid blocking
// tests, we only verify the static ID constant and accessor values that can
// be read without waiting for the async load to complete.

TEST(CourtroomScreen, StaticIdIsCourtroomString) {
    EXPECT_EQ(CourtroomScreen::ID, "courtroom");
}

TEST(CourtroomScreen, HandleEventsConsumesEnteredCourtroomEvent) {
    auto& ch = EventManager::instance().get_channel<UIEvent>();
    while (ch.get_event()) {
    } // drain

    CourtroomScreen screen("TestChar", 0);
    // Wait for the async load to finish so handle_events() doesn't race.
    // CourtroomScreen::exit() waits on the future.
    screen.exit();

    ch.publish(UIEvent(UIEventType::ENTERED_COURTROOM, "NewChar", 5));

    screen.handle_events();

    // The event should have been consumed — channel is now empty.
    EXPECT_FALSE(ch.has_events());
}

TEST(CourtroomScreen, HandleEventsCallsChangeCharacter) {
    auto& ch = EventManager::instance().get_channel<UIEvent>();
    while (ch.get_event()) {
    }

    CourtroomScreen screen("OldChar", 0);
    screen.exit(); // wait for async load

    EXPECT_EQ(screen.get_character_name(), "OldChar");
    EXPECT_EQ(screen.get_char_id(), 0);

    ch.publish(UIEvent(UIEventType::ENTERED_COURTROOM, "NewChar", 7));
    screen.handle_events();

    EXPECT_EQ(screen.get_character_name(), "NewChar");
    EXPECT_EQ(screen.get_char_id(), 7);
}

TEST(CourtroomScreen, HandleEventsIgnoresNonCourtroomEvents) {
    auto& ch = EventManager::instance().get_channel<UIEvent>();
    while (ch.get_event()) {
    }

    CourtroomScreen screen("TestChar", 0);
    screen.exit();

    ch.publish(UIEvent(UIEventType::CHAR_LOADING_DONE));
    screen.handle_events();

    // CHAR_LOADING_DONE should be consumed (drained) but not acted upon.
    EXPECT_FALSE(ch.has_events());
    // Character should be unchanged.
    EXPECT_EQ(screen.get_character_name(), "TestChar");
    EXPECT_EQ(screen.get_char_id(), 0);
}

TEST(CourtroomScreen, HandleEventsWithNoEventsDoesNotCrash) {
    auto& ch = EventManager::instance().get_channel<UIEvent>();
    while (ch.get_event()) {
    }

    CourtroomScreen screen("TestChar", 0);
    screen.exit();

    EXPECT_NO_FATAL_FAILURE(screen.handle_events());
}

// All three screen types have distinct IDs.
TEST(Screens, AllScreenIdsAreDistinct) {
    EXPECT_NE(ServerListScreen::ID, CharSelectScreen::ID);
    EXPECT_NE(ServerListScreen::ID, CourtroomScreen::ID);
    EXPECT_NE(CharSelectScreen::ID, CourtroomScreen::ID);
}

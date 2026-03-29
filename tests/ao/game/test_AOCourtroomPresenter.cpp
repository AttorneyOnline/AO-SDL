#include "ao/game/AOCourtroomPresenter.h"

#include "ao/event/ICLogEvent.h"
#include "event/BackgroundEvent.h"
#include "event/EventManager.h"
#include "event/ICMessageEvent.h"
#include "render/RenderState.h"

#include <gtest/gtest.h>

namespace {

template <typename T>
static void drain(EventChannel<T>& ch) {
    while (ch.get_event()) {
    }
}

static void drain_presenter_channels() {
    auto& em = EventManager::instance();
    drain(em.get_channel<ICMessageEvent>());
    drain(em.get_channel<BackgroundEvent>());
    drain(em.get_channel<ICLogEvent>());
}

class AOCourtroomPresenterTest : public ::testing::Test {
  protected:
    AOCourtroomPresenter presenter;

    void SetUp() override {
        drain_presenter_channels();
        presenter.init();
    }

    void TearDown() override {
        drain_presenter_channels();
    }

    // Publish an IC message event and tick once to process it
    void send_ic(const std::string& character, const std::string& message, const std::string& side,
                 EmoteMod mod = EmoteMod::IDLE, bool slide = false) {
        EventManager::instance().get_channel<ICMessageEvent>().publish(
            ICMessageEvent(character, "normal", "", message, character, side, mod, DeskMod::CHAT, false, 0, 0, 0, false,
                           false, false, "", "", 0, false, "", false, slide));
        presenter.tick(16);
    }

    void send_background(const std::string& bg, const std::string& pos = "") {
        EventManager::instance().get_channel<BackgroundEvent>().publish(BackgroundEvent(bg, pos));
        presenter.tick(16);
    }
};

} // namespace

// ---------------------------------------------------------------------------
// Basic lifecycle
// ---------------------------------------------------------------------------

TEST_F(AOCourtroomPresenterTest, TickProducesRenderState) {
    auto state = presenter.tick(16);
    auto& groups = state.get_layer_groups();
    EXPECT_GE(groups.size(), 1u);
}

TEST_F(AOCourtroomPresenterTest, TickProfileHasSections) {
    auto profile = presenter.tick_profile();
    EXPECT_FALSE(profile.empty());
}

// ---------------------------------------------------------------------------
// IC message handling
// ---------------------------------------------------------------------------

TEST_F(AOCourtroomPresenterTest, ICMessagePublishesICLog) {
    auto& log_ch = EventManager::instance().get_channel<ICLogEvent>();
    drain(log_ch);

    send_ic("Phoenix", "Hello!", "def");

    auto ev = log_ch.get_event();
    ASSERT_TRUE(ev.has_value());
    EXPECT_EQ(ev->get_message(), "Hello!");
}

TEST_F(AOCourtroomPresenterTest, BlankMessageDoesNotCrash) {
    EXPECT_NO_FATAL_FAILURE(send_ic("Phoenix", "", "def"));
}

TEST_F(AOCourtroomPresenterTest, WhitespaceOnlyMessageDoesNotCrash) {
    EXPECT_NO_FATAL_FAILURE(send_ic("Phoenix", "   ", "def"));
}

// ---------------------------------------------------------------------------
// Background / area changes
// ---------------------------------------------------------------------------

TEST_F(AOCourtroomPresenterTest, BackgroundEventClearsICState) {
    send_ic("Phoenix", "Hello!", "def");
    send_background("gs4", "wit");

    // After area change, tick should produce a valid state with no character
    auto state = presenter.tick(16);
    EXPECT_GE(state.get_layer_groups().size(), 1u);
}

// ---------------------------------------------------------------------------
// Position changes without slide
// ---------------------------------------------------------------------------

TEST_F(AOCourtroomPresenterTest, PositionChangeWithoutSlideProducesSingleGroup) {
    send_ic("Phoenix", "Hello", "def");
    send_ic("Edgeworth", "Objection", "pro"); // no slide flag

    auto state = presenter.tick(16);
    EXPECT_EQ(state.get_layer_groups().size(), 1u);
}

// ---------------------------------------------------------------------------
// Slide transitions
// ---------------------------------------------------------------------------

TEST_F(AOCourtroomPresenterTest, SlideProducesTwoGroups) {
    // Both messages in the same tick — queue processes first immediately,
    // second is queued. Tick enough for first to finish, then second triggers slide.
    EventManager::instance().get_channel<ICMessageEvent>().publish(
        ICMessageEvent("Phoenix", "normal", "", "Hi", "Phoenix", "def", EmoteMod::IDLE, DeskMod::CHAT, false, 0, 0, 0,
                       false, false, false, "", "", 0, false, "", false, false));
    EventManager::instance().get_channel<ICMessageEvent>().publish(
        ICMessageEvent("Edgeworth", "normal", "", "Objection", "Edgeworth", "pro", EmoteMod::IDLE, DeskMod::CHAT, false,
                       1, 0, 0, false, false, false, "", "", 0, false, "", false, true));

    // First tick: both enqueued, first dequeued and played
    presenter.tick(16);

    // Tick until first message finishes and second is dequeued with slide
    bool found_two_groups = false;
    for (int i = 0; i < 100; i++) {
        auto state = presenter.tick(16);
        if (state.get_layer_groups().size() == 2u) {
            found_two_groups = true;
            break;
        }
    }
    EXPECT_TRUE(found_two_groups);
}

TEST_F(AOCourtroomPresenterTest, SlideBlocksQueueAdvancement) {
    send_ic("Phoenix", "Hello", "def");

    // Send slide + a queued message
    EventManager::instance().get_channel<ICMessageEvent>().publish(
        ICMessageEvent("Edgeworth", "normal", "", "Objection", "Edgeworth", "pro", EmoteMod::IDLE, DeskMod::CHAT, false,
                       1, 0, 0, false, false, false, "", "", 0, false, "", false, true));
    EventManager::instance().get_channel<ICMessageEvent>().publish(
        ICMessageEvent("Judge", "normal", "", "Order!", "Judge", "jud", EmoteMod::IDLE, DeskMod::CHAT, false, 2, 0, 0,
                       false, false, false, "", "", 0, false, "", false, false));
    presenter.tick(16);

    // Tick a few frames — slide should still be active, Judge's message queued
    for (int i = 0; i < 10; i++)
        presenter.tick(16);

    // After enough time for slide to finish (300+600+300=1200ms), queue should advance
    for (int i = 0; i < 100; i++)
        presenter.tick(16);

    // Should eventually settle to single group (slide done)
    auto state = presenter.tick(16);
    EXPECT_EQ(state.get_layer_groups().size(), 1u);
}

TEST_F(AOCourtroomPresenterTest, SlideWithZoomEmoteDoesNotSlide) {
    send_ic("Phoenix", "Hello", "def");
    send_ic("Edgeworth", "Objection", "pro", EmoteMod::ZOOM, true);

    auto state = presenter.tick(16);
    EXPECT_EQ(state.get_layer_groups().size(), 1u); // no slide despite flag
}

TEST_F(AOCourtroomPresenterTest, SlideSamePositionDoesNotSlide) {
    send_ic("Phoenix", "Hello", "def");
    send_ic("Phoenix", "More text", "def", EmoteMod::IDLE, true);

    auto state = presenter.tick(16);
    EXPECT_EQ(state.get_layer_groups().size(), 1u);
}

// ---------------------------------------------------------------------------
// Local player slide
// ---------------------------------------------------------------------------

TEST_F(AOCourtroomPresenterTest, LocalPlayerSlideForced) {
    presenter.set_local_player(42, true);

    // Both messages together — second is ours without slide flag but local slide enabled
    EventManager::instance().get_channel<ICMessageEvent>().publish(
        ICMessageEvent("Phoenix", "normal", "", "Hi", "Phoenix", "def", EmoteMod::IDLE, DeskMod::CHAT, false, 42, 0, 0,
                       false, false, false, "", "", 0, false, "", false, false));
    EventManager::instance().get_channel<ICMessageEvent>().publish(
        ICMessageEvent("Phoenix", "normal", "", "Objection", "Phoenix", "pro", EmoteMod::IDLE, DeskMod::CHAT, false, 42,
                       0, 0, false, false, false, "", "", 0, false, "", false, false));
    presenter.tick(16);

    bool found_two_groups = false;
    for (int i = 0; i < 100; i++) {
        auto state = presenter.tick(16);
        if (state.get_layer_groups().size() == 2u) {
            found_two_groups = true;
            break;
        }
    }
    EXPECT_TRUE(found_two_groups);
}

TEST_F(AOCourtroomPresenterTest, LocalPlayerSlideNotForcedForOthers) {
    presenter.set_local_player(42, true);
    send_ic("Phoenix", "Hello", "def");

    // Someone else's message without slide flag
    EventManager::instance().get_channel<ICMessageEvent>().publish(
        ICMessageEvent("Edgeworth", "normal", "", "Objection", "Edgeworth", "pro", EmoteMod::IDLE, DeskMod::CHAT, false,
                       99, 0, 0, false, false, false, "", "", 0, false, "", false, false));
    presenter.tick(16);

    auto state = presenter.tick(16);
    EXPECT_EQ(state.get_layer_groups().size(), 1u); // no slide for char_id 99
}

// ---------------------------------------------------------------------------
// Courtroom active flag
// ---------------------------------------------------------------------------

TEST_F(AOCourtroomPresenterTest, SetCourtroomActiveDoesNotCrash) {
    presenter.set_courtroom_active(true);
    EXPECT_NO_FATAL_FAILURE(presenter.tick(16));
    presenter.set_courtroom_active(false);
    EXPECT_NO_FATAL_FAILURE(presenter.tick(16));
}

// ---------------------------------------------------------------------------
// Multiple rapid messages
// ---------------------------------------------------------------------------

TEST_F(AOCourtroomPresenterTest, RapidMessagesDoNotCrash) {
    for (int i = 0; i < 20; i++) {
        send_ic("Phoenix", "Message " + std::to_string(i), i % 2 == 0 ? "def" : "pro");
    }
    auto state = presenter.tick(16);
    EXPECT_GE(state.get_layer_groups().size(), 1u);
}

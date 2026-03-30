#include "ui/widgets/MusicAreaWidget.h"

#include "ui/widgets/CourtroomState.h"
#include "ui/widgets/ICMessageState.h"

#include "event/EventManager.h"
#include "event/MusicListEvent.h"
#include "event/NowPlayingEvent.h"

#include <imgui.h>

#include <gtest/gtest.h>

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------

template <typename T>
static void drain(EventChannel<T>& ch) {
    while (ch.get_event()) {
    }
}

static void drain_all() {
    drain(EventManager::instance().get_channel<MusicListEvent>());
    drain(EventManager::instance().get_channel<NowPlayingEvent>());
}

// ---------------------------------------------------------------------------
// Fixture: provides a fresh MusicAreaWidget with clean singleton state and
// a headless ImGui context for render() calls.
// ---------------------------------------------------------------------------

class MusicAreaWidgetTest : public ::testing::Test {
  protected:
    void SetUp() override {
        CourtroomState::instance().reset();
        drain_all();

        ctx_ = ImGui::CreateContext();
        ImGuiIO& io = ImGui::GetIO();
        io.DisplaySize = ImVec2(800, 600);
        io.DeltaTime = 1.0f / 60.0f;
        // Build a minimal font atlas so ImGui doesn't assert
        unsigned char* pixels;
        int w, h;
        io.Fonts->GetTexDataAsRGBA32(&pixels, &w, &h);

        ic_state_ = {};
        widget_ = std::make_unique<MusicAreaWidget>(&ic_state_);
    }

    void TearDown() override {
        widget_.reset();
        ImGui::DestroyContext(ctx_);
        CourtroomState::instance().reset();
        drain_all();
    }

    /// Call render() inside a valid ImGui frame.
    void render_frame() {
        ImGui::NewFrame();
        widget_->render();
        ImGui::EndFrame();
    }

    ImGuiContext* ctx_ = nullptr;
    ICMessageState ic_state_;
    std::unique_ptr<MusicAreaWidget> widget_;
};

// ===========================================================================
// handle_events: MusicListEvent processing
// ===========================================================================

TEST_F(MusicAreaWidgetTest, HandleEventsPopulatesCourtroomStateTracks) {
    auto& ch = EventManager::instance().get_channel<MusicListEvent>();
    ch.publish(MusicListEvent({"Lobby", "Room 1"}, {"Category", "song.opus", "track.mp3"}));

    widget_->handle_events();

    auto& cs = CourtroomState::instance();
    ASSERT_EQ(cs.tracks.size(), 3u);
    EXPECT_EQ(cs.tracks[0], "Category");
    EXPECT_EQ(cs.tracks[1], "song.opus");
    EXPECT_EQ(cs.tracks[2], "track.mp3");
}

TEST_F(MusicAreaWidgetTest, HandleEventsPopulatesAreas) {
    auto& ch = EventManager::instance().get_channel<MusicListEvent>();
    ch.publish(MusicListEvent({"Lobby", "Room 1"}, {}));

    widget_->handle_events();

    auto& cs = CourtroomState::instance();
    ASSERT_EQ(cs.areas.size(), 2u);
    EXPECT_EQ(cs.areas[0], "Lobby");
    EXPECT_EQ(cs.areas[1], "Room 1");
}

TEST_F(MusicAreaWidgetTest, HandleEventsPartialOnlyUpdatesTracks) {
    auto& ch = EventManager::instance().get_channel<MusicListEvent>();
    // Full event first to set areas
    ch.publish(MusicListEvent({"Lobby"}, {"old.opus"}));
    // Partial event with only tracks
    ch.publish(MusicListEvent({}, {"new.opus", "another.mp3"}, /*partial=*/true));

    widget_->handle_events();

    auto& cs = CourtroomState::instance();
    EXPECT_EQ(cs.areas.size(), 1u);
    EXPECT_EQ(cs.areas[0], "Lobby");
    ASSERT_EQ(cs.tracks.size(), 2u);
    EXPECT_EQ(cs.tracks[0], "new.opus");
}

TEST_F(MusicAreaWidgetTest, HandleEventsWithNoEventsIsNoOp) {
    widget_->handle_events();

    auto& cs = CourtroomState::instance();
    EXPECT_TRUE(cs.tracks.empty());
    EXPECT_TRUE(cs.areas.empty());
}

// ===========================================================================
// Crash scenario: Issue #85
//
// The original crash: after a character change, a new MusicAreaWidget is
// created with empty caches, but CourtroomState.tracks retains stale data.
// render() iterates cs.tracks.size() and indexes into the empty local
// vectors — SIGSEGV.
//
// The fix: render() detects the size mismatch and calls
// rebuild_track_caches() before accessing the local vectors.
// ===========================================================================

TEST_F(MusicAreaWidgetTest, RenderWithStaleSingletonStateDoesNotCrash) {
    // Step 1: Simulate the "previous session" — populate CourtroomState with
    // a music list as if an older MusicAreaWidget had processed it.
    auto& cs = CourtroomState::instance();
    cs.tracks = {"Category", "song1.opus", "song2.opus", "song3.mp3"};
    cs.areas = {"Lobby"};
    cs.area_players = {5};
    cs.area_status = {"CASING"};
    cs.area_cm = {"FREE"};
    cs.area_lock = {"FREE"};

    // Step 2: Create a NEW widget, simulating what happens after a character
    // change recreates CourtroomController and MusicAreaWidget.
    // The new widget's local caches (tracks_trimmed_, tracks_lower_) are empty,
    // but CourtroomState.tracks still has 4 entries from step 1.
    ic_state_ = {};
    widget_ = std::make_unique<MusicAreaWidget>(&ic_state_);

    // Step 3: Call handle_events() with NO pending events — this is exactly
    // what happens after widget recreation, since the MusicListEvents were
    // already consumed by the old widget.
    widget_->handle_events();

    // Step 4: Call render(). Before the fix, this would SIGSEGV because render()
    // loops over cs.tracks.size() (4) but accesses empty tracks_trimmed_[i].
    // After the fix, the size-mismatch guard calls rebuild_track_caches() first.
    EXPECT_NO_FATAL_FAILURE(render_frame());
}

TEST_F(MusicAreaWidgetTest, RenderSyncsTrackCachesFromSingleton) {
    // Pre-populate CourtroomState (simulating stale state from a previous widget)
    auto& cs = CourtroomState::instance();
    cs.tracks = {"Jazz", "smooth.opus", "cool.mp3"};

    // New widget — caches are empty
    ic_state_ = {};
    widget_ = std::make_unique<MusicAreaWidget>(&ic_state_);

    // render() should sync the caches before accessing them.
    // After render, the widget should be safe to render again (caches populated).
    EXPECT_NO_FATAL_FAILURE(render_frame());
    EXPECT_NO_FATAL_FAILURE(render_frame());
}

TEST_F(MusicAreaWidgetTest, RenderWithEmptyStateDoesNotCrash) {
    // Both singleton and widget are empty — render should be a no-op.
    EXPECT_NO_FATAL_FAILURE(render_frame());
}

TEST_F(MusicAreaWidgetTest, RenderAfterHandleEventsDoesNotCrash) {
    // Normal flow: events arrive, handle_events processes them, render displays.
    auto& ch = EventManager::instance().get_channel<MusicListEvent>();
    ch.publish(MusicListEvent({"Lobby", "Room 1"},
                              {"Category", "track1.opus", "track2.mp3", "Another Category", "track3.wav"}));

    widget_->handle_events();
    EXPECT_NO_FATAL_FAILURE(render_frame());
}

// ===========================================================================
// Simulated "second character change" — multiple widget recreations
// ===========================================================================

TEST_F(MusicAreaWidgetTest, MultipleWidgetRecreationsDoNotCrash) {
    auto& ch = EventManager::instance().get_channel<MusicListEvent>();

    // First widget processes events
    ch.publish(MusicListEvent({"Area"}, {"Cat", "a.opus", "b.opus"}));
    widget_->handle_events();
    EXPECT_NO_FATAL_FAILURE(render_frame());

    // Simulate character change: new widget, no new events
    ic_state_ = {};
    widget_ = std::make_unique<MusicAreaWidget>(&ic_state_);
    widget_->handle_events(); // no events pending
    EXPECT_NO_FATAL_FAILURE(render_frame());

    // Simulate another character change
    ic_state_ = {};
    widget_ = std::make_unique<MusicAreaWidget>(&ic_state_);
    EXPECT_NO_FATAL_FAILURE(render_frame());
}

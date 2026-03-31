#include "game/GameThread.h"

#include <atomic>
#include <chrono>
#include <gtest/gtest.h>

// ---------------------------------------------------------------------------
// Minimal mock presenter that counts ticks
// ---------------------------------------------------------------------------

class MockPresenter : public IScenePresenter {
  public:
    void init() override {
        ++init_count;
    }

    RenderState tick(uint64_t /*delta_ms*/) override {
        ++tick_count;
        return {};
    }

    std::atomic<int> init_count{0};
    std::atomic<int> tick_count{0};
};

// ---------------------------------------------------------------------------
// Construction / destruction
// ---------------------------------------------------------------------------

TEST(GameThread, ConstructAndStopImmediately) {
    StateBuffer buf;
    MockPresenter presenter;
    GameThread gt(buf, presenter);
    gt.stop();
    EXPECT_GE(presenter.init_count.load(), 1);
}

TEST(GameThread, StopIsIdempotent) {
    StateBuffer buf;
    MockPresenter presenter;
    GameThread gt(buf, presenter);
    gt.stop();
    gt.stop();
}

// ---------------------------------------------------------------------------
// jthread auto-join: destructor must not call std::terminate
// ---------------------------------------------------------------------------

TEST(GameThread, DestructorAutoJoinsWithoutExplicitStop) {
    // jthread's destructor calls request_stop() + join().
    // Before the jthread migration, forgetting stop() would call
    // std::terminate. This test verifies the auto-join works.
    StateBuffer buf;
    MockPresenter presenter;
    {
        GameThread gt(buf, presenter);
        // Intentionally NOT calling gt.stop() — destructor handles it.
    }
    SUCCEED();
}

TEST(GameThread, DestructorAutoJoinsAfterTicking) {
    StateBuffer buf;
    MockPresenter presenter;
    {
        GameThread gt(buf, presenter);
        // Let it tick a few times.
        std::this_thread::sleep_for(std::chrono::milliseconds(150));
        // Destructor auto-joins without explicit stop().
    }
    EXPECT_GT(presenter.tick_count.load(), 0);
}

// ---------------------------------------------------------------------------
// Stop token cooperative cancellation
// ---------------------------------------------------------------------------

TEST(GameThread, StopCompletesInBoundedTime) {
    StateBuffer buf;
    MockPresenter presenter;
    GameThread gt(buf, presenter);

    // Let the thread run briefly.
    std::this_thread::sleep_for(std::chrono::milliseconds(50));

    auto start = std::chrono::steady_clock::now();
    gt.stop();
    auto elapsed = std::chrono::steady_clock::now() - start;

    // stop() should return quickly — the loop checks stop_token each iteration.
    EXPECT_LT(elapsed, std::chrono::seconds(2));
}

TEST(GameThread, TicksBeforeStop) {
    StateBuffer buf;
    MockPresenter presenter;
    GameThread gt(buf, presenter);

    std::this_thread::sleep_for(std::chrono::milliseconds(200));
    gt.stop();

    EXPECT_GT(presenter.tick_count.load(), 0);
}

// ---------------------------------------------------------------------------
// Multiple sequential instances
// ---------------------------------------------------------------------------

TEST(GameThread, MultipleSequentialInstances) {
    StateBuffer buf;
    MockPresenter presenter;

    for (int i = 0; i < 3; ++i) {
        GameThread gt(buf, presenter);
        gt.stop();
    }

    EXPECT_GE(presenter.init_count.load(), 3);
}

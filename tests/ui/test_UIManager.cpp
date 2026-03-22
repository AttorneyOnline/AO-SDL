#include "ui/UIManager.h"

#include <gtest/gtest.h>
#include <memory>
#include <string>

class MockScreen : public Screen {
  public:
    void enter(ScreenController& c) override {
        entered = true;
        enter_count++;
        controller_ = &c;
    }
    void exit() override {
        exited = true;
        exit_count++;
    }
    void handle_events() override { events_handled++; }
    const std::string& screen_id() const override { return id; }

    std::string id = "mock";
    bool entered = false;
    bool exited = false;
    int enter_count = 0;
    int exit_count = 0;
    int events_handled = 0;
    ScreenController* controller_ = nullptr;
};

// --- Initial state ---

TEST(UIManager, InitialActiveScreenIsNull) {
    UIManager mgr;
    EXPECT_EQ(mgr.active_screen(), nullptr);
}

TEST(UIManager, HandleEventsOnEmptyStackDoesNotCrash) {
    UIManager mgr;
    mgr.handle_events(); // should be a no-op
}

// --- push_screen ---

TEST(UIManager, PushScreenMakesItActive) {
    UIManager mgr;
    auto screen = std::make_unique<MockScreen>();
    auto* raw = screen.get();
    mgr.push_screen(std::move(screen));
    EXPECT_EQ(mgr.active_screen(), raw);
}

TEST(UIManager, PushScreenCallsEnter) {
    UIManager mgr;
    auto screen = std::make_unique<MockScreen>();
    auto* raw = screen.get();
    mgr.push_screen(std::move(screen));
    EXPECT_TRUE(raw->entered);
    EXPECT_EQ(raw->enter_count, 1);
}

TEST(UIManager, PushScreenPassesControllerToEnter) {
    UIManager mgr;
    auto screen = std::make_unique<MockScreen>();
    auto* raw = screen.get();
    mgr.push_screen(std::move(screen));
    EXPECT_EQ(raw->controller_, &mgr);
}

TEST(UIManager, PushSecondScreenExitsFirst) {
    UIManager mgr;
    auto first = std::make_unique<MockScreen>();
    first->id = "first";
    auto* first_raw = first.get();
    mgr.push_screen(std::move(first));

    auto second = std::make_unique<MockScreen>();
    second->id = "second";
    auto* second_raw = second.get();
    mgr.push_screen(std::move(second));

    EXPECT_TRUE(first_raw->exited);
    EXPECT_EQ(first_raw->exit_count, 1);
    EXPECT_TRUE(second_raw->entered);
    EXPECT_EQ(mgr.active_screen(), second_raw);
}

TEST(UIManager, PushMultipleScreensOnlyTopIsActive) {
    UIManager mgr;
    auto a = std::make_unique<MockScreen>();
    a->id = "a";
    auto b = std::make_unique<MockScreen>();
    b->id = "b";
    auto c = std::make_unique<MockScreen>();
    c->id = "c";
    auto* c_raw = c.get();

    mgr.push_screen(std::move(a));
    mgr.push_screen(std::move(b));
    mgr.push_screen(std::move(c));

    EXPECT_EQ(mgr.active_screen(), c_raw);
    EXPECT_EQ(mgr.active_screen()->screen_id(), "c");
}

// --- pop_screen ---

TEST(UIManager, PopScreenRemovesActive) {
    UIManager mgr;
    auto screen = std::make_unique<MockScreen>();
    auto* raw = screen.get();
    mgr.push_screen(std::move(screen));

    mgr.pop_screen();
    EXPECT_EQ(mgr.active_screen(), nullptr);
    // raw is now dangling, but we verified exit was called before destruction
}

TEST(UIManager, PopScreenCallsExitOnPoppedScreen) {
    UIManager mgr;
    auto screen = std::make_unique<MockScreen>();
    // We need to track exit before the screen is destroyed.
    // Use a flag captured externally.
    bool exit_called = false;

    class ExitTracker : public Screen {
      public:
        explicit ExitTracker(bool& flag) : flag_(flag) {}
        void enter(ScreenController&) override {}
        void exit() override { flag_ = true; }
        void handle_events() override {}
        const std::string& screen_id() const override { return id_; }

      private:
        bool& flag_;
        std::string id_ = "tracker";
    };

    mgr.push_screen(std::make_unique<ExitTracker>(exit_called));
    EXPECT_FALSE(exit_called);
    mgr.pop_screen();
    EXPECT_TRUE(exit_called);
}

TEST(UIManager, PopScreenReactivatesScreenUnderneath) {
    UIManager mgr;
    auto first = std::make_unique<MockScreen>();
    first->id = "first";
    auto* first_raw = first.get();
    mgr.push_screen(std::move(first));

    auto second = std::make_unique<MockScreen>();
    second->id = "second";
    mgr.push_screen(std::move(second));

    // first was entered once on push, then exited when second was pushed
    EXPECT_EQ(first_raw->enter_count, 1);
    EXPECT_EQ(first_raw->exit_count, 1);

    mgr.pop_screen();

    // first should be re-entered
    EXPECT_EQ(mgr.active_screen(), first_raw);
    EXPECT_EQ(first_raw->enter_count, 2);
}

TEST(UIManager, PopOnEmptyStackDoesNotCrash) {
    UIManager mgr;
    mgr.pop_screen(); // should be a no-op
    EXPECT_EQ(mgr.active_screen(), nullptr);
}

// --- active_screen ---

TEST(UIManager, ActiveScreenReturnsCorrectPointer) {
    UIManager mgr;
    auto screen = std::make_unique<MockScreen>();
    screen->id = "test_screen";
    auto* raw = screen.get();
    mgr.push_screen(std::move(screen));

    Screen* active = mgr.active_screen();
    ASSERT_NE(active, nullptr);
    EXPECT_EQ(active, raw);
    EXPECT_EQ(active->screen_id(), "test_screen");
}

// --- handle_events ---

TEST(UIManager, HandleEventsDispatchesToActiveScreen) {
    UIManager mgr;
    auto screen = std::make_unique<MockScreen>();
    auto* raw = screen.get();
    mgr.push_screen(std::move(screen));

    EXPECT_EQ(raw->events_handled, 0);
    mgr.handle_events();
    EXPECT_EQ(raw->events_handled, 1);
    mgr.handle_events();
    EXPECT_EQ(raw->events_handled, 2);
}

TEST(UIManager, HandleEventsOnlyDispatchesToTopScreen) {
    UIManager mgr;
    auto first = std::make_unique<MockScreen>();
    first->id = "first";
    auto* first_raw = first.get();
    mgr.push_screen(std::move(first));

    auto second = std::make_unique<MockScreen>();
    second->id = "second";
    auto* second_raw = second.get();
    mgr.push_screen(std::move(second));

    mgr.handle_events();
    EXPECT_EQ(first_raw->events_handled, 0);
    EXPECT_EQ(second_raw->events_handled, 1);
}

// --- Screen lifecycle ordering ---

TEST(UIManager, PushPopPushLifecycle) {
    UIManager mgr;
    auto a = std::make_unique<MockScreen>();
    a->id = "a";
    auto* a_raw = a.get();
    mgr.push_screen(std::move(a));

    // Push b on top of a
    auto b = std::make_unique<MockScreen>();
    b->id = "b";
    auto* b_raw = b.get();
    mgr.push_screen(std::move(b));

    EXPECT_EQ(a_raw->enter_count, 1);
    EXPECT_EQ(a_raw->exit_count, 1);
    EXPECT_EQ(b_raw->enter_count, 1);
    EXPECT_EQ(b_raw->exit_count, 0);

    // Pop b, reactivating a
    mgr.pop_screen();
    EXPECT_EQ(a_raw->enter_count, 2);
    EXPECT_EQ(mgr.active_screen(), a_raw);

    // Push c on top of a
    auto c = std::make_unique<MockScreen>();
    c->id = "c";
    auto* c_raw = c.get();
    mgr.push_screen(std::move(c));

    EXPECT_EQ(a_raw->exit_count, 2);
    EXPECT_EQ(c_raw->enter_count, 1);
    EXPECT_EQ(mgr.active_screen(), c_raw);
}

// --- pop_to_root ---

TEST(UIManager, PopToRootLeavesOnlyBottomScreen) {
    UIManager mgr;
    auto root = std::make_unique<MockScreen>();
    root->id = "root";
    auto* root_raw = root.get();
    mgr.push_screen(std::move(root));

    auto mid = std::make_unique<MockScreen>();
    mid->id = "mid";
    mgr.push_screen(std::move(mid));

    auto top = std::make_unique<MockScreen>();
    top->id = "top";
    mgr.push_screen(std::move(top));

    mgr.pop_to_root();

    EXPECT_EQ(mgr.active_screen(), root_raw);
    EXPECT_EQ(root_raw->screen_id(), "root");
}

TEST(UIManager, PopToRootCallsEnterOnRoot) {
    UIManager mgr;
    auto root = std::make_unique<MockScreen>();
    root->id = "root";
    auto* root_raw = root.get();
    mgr.push_screen(std::move(root));

    auto top = std::make_unique<MockScreen>();
    top->id = "top";
    mgr.push_screen(std::move(top));

    // root was entered once on initial push, then exited when top was pushed
    EXPECT_EQ(root_raw->enter_count, 1);

    mgr.pop_to_root();

    // root should be re-entered after pop_to_root
    EXPECT_EQ(root_raw->enter_count, 2);
}

TEST(UIManager, PopToRootOnSingleScreenIsNoOp) {
    UIManager mgr;
    auto screen = std::make_unique<MockScreen>();
    auto* raw = screen.get();
    mgr.push_screen(std::move(screen));

    int enter_before = raw->enter_count;
    mgr.pop_to_root();

    EXPECT_EQ(mgr.active_screen(), raw);
    // enter is still called even when already at root (implementation detail)
    EXPECT_EQ(raw->enter_count, enter_before + 1);
}

TEST(UIManager, PopToRootOnEmptyStackDoesNotCrash) {
    UIManager mgr;
    mgr.pop_to_root(); // should be a no-op
    EXPECT_EQ(mgr.active_screen(), nullptr);
}

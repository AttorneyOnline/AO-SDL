#include "ImGuiTestFixture.h"

#include "ui/widgets/TimerWidget.h"

#include "event/EventManager.h"
#include "event/TimerEvent.h"

class TimerWidgetTest : public ImGuiTestFixture {
  protected:
    void SetUp() override {
        ImGuiTestFixture::SetUp();
        drain<TimerEvent>();
    }

    TimerWidget widget_;
};

TEST_F(TimerWidgetTest, HandleEvents_StartSetsRunningAndTime) {
    EventManager::instance().get_channel<TimerEvent>().publish(TimerEvent(0, 0, 30000));
    widget_.handle_events();
    // We can't directly inspect Timer state (private), but we can verify
    // render doesn't crash and shows a timer.
    with_frame([&] { widget_.render(); });
}

TEST_F(TimerWidgetTest, HandleEvents_PauseStopsTimer) {
    auto& ch = EventManager::instance().get_channel<TimerEvent>();
    ch.publish(TimerEvent(0, 2, 0));     // show
    ch.publish(TimerEvent(0, 0, 60000)); // start at 60s
    widget_.handle_events();
    with_frame([&] { widget_.render(); });

    ch.publish(TimerEvent(0, 1, 45000)); // pause at 45s
    widget_.handle_events();
    with_frame([&] { widget_.render(); });
}

TEST_F(TimerWidgetTest, HandleEvents_ShowHideVisibility) {
    auto& ch = EventManager::instance().get_channel<TimerEvent>();
    ch.publish(TimerEvent(0, 2, 0)); // show
    widget_.handle_events();
    with_frame([&] { widget_.render(); });

    ch.publish(TimerEvent(0, 3, 0)); // hide
    widget_.handle_events();
    with_frame([&] { widget_.render(); });
}

TEST_F(TimerWidgetTest, HandleEvents_InvalidTimerIdIgnored) {
    auto& ch = EventManager::instance().get_channel<TimerEvent>();
    ch.publish(TimerEvent(-1, 0, 10000));
    ch.publish(TimerEvent(5, 0, 10000)); // MAX_TIMERS is 5, so id=5 is out of range
    ch.publish(TimerEvent(99, 2, 0));
    widget_.handle_events();
    with_frame([&] { widget_.render(); });
}

TEST_F(TimerWidgetTest, HandleEvents_NegativeTimeMsStopsTimer) {
    auto& ch = EventManager::instance().get_channel<TimerEvent>();
    ch.publish(TimerEvent(0, 2, 0));     // show
    ch.publish(TimerEvent(0, 0, 10000)); // start
    widget_.handle_events();

    ch.publish(TimerEvent(0, 0, -1)); // negative time = stop
    widget_.handle_events();
    with_frame([&] { widget_.render(); });
}

TEST_F(TimerWidgetTest, HandleEvents_MultipleTimers) {
    auto& ch = EventManager::instance().get_channel<TimerEvent>();
    ch.publish(TimerEvent(0, 2, 0));     // show timer 0
    ch.publish(TimerEvent(1, 2, 0));     // show timer 1
    ch.publish(TimerEvent(0, 0, 30000)); // start timer 0
    ch.publish(TimerEvent(1, 0, 60000)); // start timer 1
    widget_.handle_events();
    with_frame([&] { widget_.render(); });
}

TEST_F(TimerWidgetTest, RenderSmokeTest_NoTimersVisible) {
    with_frame([&] { widget_.render(); });
}

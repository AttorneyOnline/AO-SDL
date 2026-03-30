#include "ImGuiTestFixture.h"

#include "ui/widgets/HealthBarWidget.h"
#include "ui/widgets/ICMessageState.h"

#include "event/EventManager.h"
#include "event/HealthBarEvent.h"
#include "event/OutgoingHealthBarEvent.h"

class HealthBarWidgetTest : public ImGuiTestFixture {
  protected:
    void SetUp() override {
        ImGuiTestFixture::SetUp();
        drain<HealthBarEvent>();
        drain<OutgoingHealthBarEvent>();
    }

    ICMessageState ic_state_;
    HealthBarWidget widget_{&ic_state_};
};

TEST_F(HealthBarWidgetTest, HandleEvents_DefenseHP) {
    EventManager::instance().get_channel<HealthBarEvent>().publish(HealthBarEvent(1, 7));
    widget_.handle_events();
    EXPECT_EQ(CourtroomState::instance().def_hp, 7);
}

TEST_F(HealthBarWidgetTest, HandleEvents_ProsecutionHP) {
    EventManager::instance().get_channel<HealthBarEvent>().publish(HealthBarEvent(2, 4));
    widget_.handle_events();
    EXPECT_EQ(CourtroomState::instance().pro_hp, 4);
}

TEST_F(HealthBarWidgetTest, HandleEvents_ClampsToRange) {
    auto& ch = EventManager::instance().get_channel<HealthBarEvent>();
    ch.publish(HealthBarEvent(1, 15)); // above max
    ch.publish(HealthBarEvent(2, -3)); // below min
    widget_.handle_events();
    EXPECT_EQ(CourtroomState::instance().def_hp, 10);
    EXPECT_EQ(CourtroomState::instance().pro_hp, 0);
}

TEST_F(HealthBarWidgetTest, HandleEvents_IndependentSides) {
    auto& ch = EventManager::instance().get_channel<HealthBarEvent>();
    ch.publish(HealthBarEvent(1, 3));
    ch.publish(HealthBarEvent(2, 8));
    widget_.handle_events();
    EXPECT_EQ(CourtroomState::instance().def_hp, 3);
    EXPECT_EQ(CourtroomState::instance().pro_hp, 8);
}

TEST_F(HealthBarWidgetTest, HandleEvents_IgnoresUnknownSide) {
    EventManager::instance().get_channel<HealthBarEvent>().publish(HealthBarEvent(99, 5));
    widget_.handle_events();
    EXPECT_EQ(CourtroomState::instance().def_hp, 0);
    EXPECT_EQ(CourtroomState::instance().pro_hp, 0);
}

TEST_F(HealthBarWidgetTest, HandleEvents_LastEventWins) {
    auto& ch = EventManager::instance().get_channel<HealthBarEvent>();
    ch.publish(HealthBarEvent(1, 3));
    ch.publish(HealthBarEvent(1, 9));
    widget_.handle_events();
    EXPECT_EQ(CourtroomState::instance().def_hp, 9);
}

TEST_F(HealthBarWidgetTest, RenderSmokeTest) {
    CourtroomState::instance().def_hp = 5;
    CourtroomState::instance().pro_hp = 3;
    with_frame([&] { widget_.render(); });
}

TEST_F(HealthBarWidgetTest, RenderSmokeTest_JudgeMode) {
    ic_state_.side_index = 3; // judge
    CourtroomState::instance().def_hp = 5;
    CourtroomState::instance().pro_hp = 3;
    with_frame([&] { widget_.render(); });
}

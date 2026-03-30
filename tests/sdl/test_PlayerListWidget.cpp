#include "ImGuiTestFixture.h"

#include "ui/widgets/PlayerListWidget.h"

#include "event/EventManager.h"
#include "event/PlayerListEvent.h"

class PlayerListWidgetTest : public ImGuiTestFixture {
  protected:
    void SetUp() override {
        ImGuiTestFixture::SetUp();
        drain<PlayerListEvent>();
    }

    PlayerListWidget widget_;
};

TEST_F(PlayerListWidgetTest, HandleEvents_AddPlayer) {
    auto& ch = EventManager::instance().get_channel<PlayerListEvent>();
    ch.publish(PlayerListEvent(PlayerListEvent::Action::ADD, 42));
    widget_.handle_events();
    EXPECT_EQ(CourtroomState::instance().players.count(42), 1);
}

TEST_F(PlayerListWidgetTest, HandleEvents_RemovePlayer) {
    auto& ch = EventManager::instance().get_channel<PlayerListEvent>();
    ch.publish(PlayerListEvent(PlayerListEvent::Action::ADD, 10));
    widget_.handle_events();
    EXPECT_EQ(CourtroomState::instance().players.size(), 1);

    ch.publish(PlayerListEvent(PlayerListEvent::Action::REMOVE, 10));
    widget_.handle_events();
    EXPECT_EQ(CourtroomState::instance().players.count(10), 0);
}

TEST_F(PlayerListWidgetTest, HandleEvents_UpdateName) {
    auto& ch = EventManager::instance().get_channel<PlayerListEvent>();
    ch.publish(PlayerListEvent(PlayerListEvent::Action::ADD, 1));
    ch.publish(PlayerListEvent(PlayerListEvent::Action::UPDATE_NAME, 1, "Phoenix"));
    widget_.handle_events();
    EXPECT_EQ(CourtroomState::instance().players[1].name, "Phoenix");
}

TEST_F(PlayerListWidgetTest, HandleEvents_UpdateCharacter) {
    auto& ch = EventManager::instance().get_channel<PlayerListEvent>();
    ch.publish(PlayerListEvent(PlayerListEvent::Action::ADD, 1));
    ch.publish(PlayerListEvent(PlayerListEvent::Action::UPDATE_CHARACTER, 1, "Phoenix"));
    widget_.handle_events();
    EXPECT_EQ(CourtroomState::instance().players[1].character, "Phoenix");
}

TEST_F(PlayerListWidgetTest, HandleEvents_UpdateCharname) {
    auto& ch = EventManager::instance().get_channel<PlayerListEvent>();
    ch.publish(PlayerListEvent(PlayerListEvent::Action::ADD, 1));
    ch.publish(PlayerListEvent(PlayerListEvent::Action::UPDATE_CHARNAME, 1, "Nick"));
    widget_.handle_events();
    EXPECT_EQ(CourtroomState::instance().players[1].charname, "Nick");
}

TEST_F(PlayerListWidgetTest, HandleEvents_UpdateArea) {
    auto& ch = EventManager::instance().get_channel<PlayerListEvent>();
    ch.publish(PlayerListEvent(PlayerListEvent::Action::ADD, 1));
    ch.publish(PlayerListEvent(PlayerListEvent::Action::UPDATE_AREA, 1, "3"));
    widget_.handle_events();
    EXPECT_EQ(CourtroomState::instance().players[1].area_id, 3);
}

TEST_F(PlayerListWidgetTest, HandleEvents_MultiplePlayersIndependent) {
    auto& ch = EventManager::instance().get_channel<PlayerListEvent>();
    ch.publish(PlayerListEvent(PlayerListEvent::Action::ADD, 1));
    ch.publish(PlayerListEvent(PlayerListEvent::Action::ADD, 2));
    ch.publish(PlayerListEvent(PlayerListEvent::Action::UPDATE_NAME, 1, "Phoenix"));
    ch.publish(PlayerListEvent(PlayerListEvent::Action::UPDATE_NAME, 2, "Edgeworth"));
    widget_.handle_events();
    EXPECT_EQ(CourtroomState::instance().players[1].name, "Phoenix");
    EXPECT_EQ(CourtroomState::instance().players[2].name, "Edgeworth");
}

TEST_F(PlayerListWidgetTest, RenderSmokeTest_Empty) {
    with_frame([&] { widget_.render(); });
}

TEST_F(PlayerListWidgetTest, RenderSmokeTest_WithPlayers) {
    auto& ch = EventManager::instance().get_channel<PlayerListEvent>();
    ch.publish(PlayerListEvent(PlayerListEvent::Action::ADD, 1));
    ch.publish(PlayerListEvent(PlayerListEvent::Action::UPDATE_NAME, 1, "Phoenix"));
    ch.publish(PlayerListEvent(PlayerListEvent::Action::UPDATE_CHARACTER, 1, "Phoenix"));
    widget_.handle_events();
    with_frame([&] { widget_.render(); });
}

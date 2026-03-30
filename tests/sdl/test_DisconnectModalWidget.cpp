#include "ImGuiTestFixture.h"

#include "ui/widgets/DisconnectModalWidget.h"

#include "event/DisconnectEvent.h"
#include "event/EventManager.h"

class DisconnectModalWidgetTest : public ImGuiTestFixture {
  protected:
    void SetUp() override {
        ImGuiTestFixture::SetUp();
        drain<DisconnectEvent>();
    }

    DisconnectModalWidget widget_;
};

TEST_F(DisconnectModalWidgetTest, InitialState) {
    EXPECT_FALSE(widget_.should_return_to_server_list());
}

TEST_F(DisconnectModalWidgetTest, HandleEvents_SetsModalOnDisconnect) {
    EventManager::instance().get_channel<DisconnectEvent>().publish(DisconnectEvent("Server closed"));
    widget_.handle_events();
    // The modal is shown via render(), but we can verify the widget accepted the event
    // by doing a render smoke test.
    with_frame([&] { widget_.render(); });
}

TEST_F(DisconnectModalWidgetTest, ClearFlag) {
    widget_.clear_flag();
    EXPECT_FALSE(widget_.should_return_to_server_list());
}

TEST_F(DisconnectModalWidgetTest, HandleEvents_EmptyReason) {
    EventManager::instance().get_channel<DisconnectEvent>().publish(DisconnectEvent(""));
    widget_.handle_events();
    with_frame([&] { widget_.render(); });
}

TEST_F(DisconnectModalWidgetTest, RenderSmokeTest_NoDisconnect) {
    with_frame([&] { widget_.render(); });
}

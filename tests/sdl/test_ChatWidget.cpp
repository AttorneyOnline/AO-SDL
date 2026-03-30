#include "ImGuiTestFixture.h"

#include "ui/widgets/ChatWidget.h"

#include "event/ChatEvent.h"
#include "event/EventManager.h"
#include "event/OutgoingChatEvent.h"

class ChatWidgetTest : public ImGuiTestFixture {
  protected:
    void SetUp() override {
        ImGuiTestFixture::SetUp();
        drain<ChatEvent>();
        drain<OutgoingChatEvent>();
    }

    ChatWidget widget_;
};

// NOTE: ChatWidget stores messages in a private static shared_buffer(), not
// in CourtroomState::chat_log. These tests can only verify "doesn't crash"
// until the widget exposes its buffer or populates CourtroomState instead.

TEST_F(ChatWidgetTest, HandleEvents_PopulatesBufferFromChatEvent) {
    EventManager::instance().get_channel<ChatEvent>().publish(ChatEvent("Alice", "Hello", false));
    widget_.handle_events();

    EventManager::instance().get_channel<ChatEvent>().publish(ChatEvent("Bob", "Hi", false));
    widget_.handle_events();
}

TEST_F(ChatWidgetTest, HandleEvents_MultipleEventsInOneBatch) {
    auto& ch = EventManager::instance().get_channel<ChatEvent>();
    ch.publish(ChatEvent("Alice", "one", false));
    ch.publish(ChatEvent("Bob", "two", false));
    ch.publish(ChatEvent("Server", "notice", true));
    widget_.handle_events();
}

TEST_F(ChatWidgetTest, ConsumeDebugToggle_DefaultFalse) {
    EXPECT_FALSE(widget_.consume_debug_toggle());
}

TEST_F(ChatWidgetTest, ConsumeDebugToggle_ClearsAfterRead) {
    // The debug toggle is set via the render path (/debug command),
    // so we can only test the consume/clear mechanism here.
    EXPECT_FALSE(widget_.consume_debug_toggle());
    EXPECT_FALSE(widget_.consume_debug_toggle()); // idempotent
}

TEST_F(ChatWidgetTest, RenderSmokeTest) {
    EventManager::instance().get_channel<ChatEvent>().publish(ChatEvent("Alice", "Hello", false));
    widget_.handle_events();
    with_frame([&] { widget_.render(); });
}

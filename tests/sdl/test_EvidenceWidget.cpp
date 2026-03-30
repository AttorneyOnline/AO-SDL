#include "ImGuiTestFixture.h"

#include "ui/widgets/EvidenceWidget.h"

#include "event/EventManager.h"
#include "event/EvidenceListEvent.h"

class EvidenceWidgetTest : public ImGuiTestFixture {
  protected:
    void SetUp() override {
        ImGuiTestFixture::SetUp();
        drain<EvidenceListEvent>();
    }

    EvidenceWidget widget_;
};

TEST_F(EvidenceWidgetTest, HandleEvents_PopulatesCourtroomState) {
    std::vector<EvidenceItem> items = {
        {"Autopsy Report", "Victim died at 9 PM", "autopsy.png"},
        {"Photo", "Crime scene photo", "photo.png"},
    };
    EventManager::instance().get_channel<EvidenceListEvent>().publish(EvidenceListEvent(items));
    widget_.handle_events();
    ASSERT_EQ(CourtroomState::instance().evidence.size(), 2);
    EXPECT_EQ(CourtroomState::instance().evidence[0].name, "Autopsy Report");
    EXPECT_EQ(CourtroomState::instance().evidence[1].description, "Crime scene photo");
}

TEST_F(EvidenceWidgetTest, HandleEvents_NewListReplacesOld) {
    auto& ch = EventManager::instance().get_channel<EvidenceListEvent>();
    ch.publish(EvidenceListEvent({{"Item1", "desc1", "img1"}}));
    widget_.handle_events();
    ASSERT_EQ(CourtroomState::instance().evidence.size(), 1);

    ch.publish(EvidenceListEvent({{"A", "", ""}, {"B", "", ""}, {"C", "", ""}}));
    widget_.handle_events();
    ASSERT_EQ(CourtroomState::instance().evidence.size(), 3);
}

TEST_F(EvidenceWidgetTest, HandleEvents_EmptyList) {
    auto& ch = EventManager::instance().get_channel<EvidenceListEvent>();
    ch.publish(EvidenceListEvent({{"Item1", "desc1", "img1"}}));
    widget_.handle_events();

    ch.publish(EvidenceListEvent({}));
    widget_.handle_events();
    EXPECT_TRUE(CourtroomState::instance().evidence.empty());
}

TEST_F(EvidenceWidgetTest, RenderSmokeTest_Empty) {
    with_frame([&] { widget_.render(); });
}

TEST_F(EvidenceWidgetTest, RenderSmokeTest_WithItems) {
    EventManager::instance().get_channel<EvidenceListEvent>().publish(
        EvidenceListEvent({{"Autopsy Report", "Victim died at 9 PM", "autopsy.png"}}));
    widget_.handle_events();
    with_frame([&] { widget_.render(); });
}

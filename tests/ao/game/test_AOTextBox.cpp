#include "ao/asset/AOAssetLibrary.h"
#include "ao/game/AOTextBox.h"

#include "asset/AssetLibrary.h"
#include "asset/MountManager.h"

#include <gtest/gtest.h>

// ---------------------------------------------------------------------------
// Fixture: AOTextBox with load() called against empty mounts.
// This populates the default color table so start_message / is_talking work,
// but no font is loaded so render() is a no-op.
// ---------------------------------------------------------------------------

namespace {

class AOTextBoxTest : public ::testing::Test {
  protected:
    MountManager mounts;
    AssetLibrary engine_assets{mounts, 0};
    AOAssetLibrary ao_assets{engine_assets};
    AOTextBox box;

    void SetUp() override {
        box.load(ao_assets);
    }
};

} // namespace

// ---------------------------------------------------------------------------
// Default / initial state
// ---------------------------------------------------------------------------

TEST_F(AOTextBoxTest, DefaultStateIsInactive) {
    AOTextBox fresh;
    EXPECT_EQ(fresh.text_state(), AOTextBox::TextState::INACTIVE);
}

// ---------------------------------------------------------------------------
// start_message
// ---------------------------------------------------------------------------

TEST_F(AOTextBoxTest, StartMessageWithEmptyMessageSetsStateToInactive) {
    box.start_message("Phoenix", "", 0);
    EXPECT_EQ(box.text_state(), AOTextBox::TextState::INACTIVE);
}

TEST_F(AOTextBoxTest, StartMessageWithNonEmptyMessageSetsStateToTicking) {
    box.start_message("Phoenix", "Hello world", 0);
    EXPECT_EQ(box.text_state(), AOTextBox::TextState::TICKING);
}

// ---------------------------------------------------------------------------
// tick — inactive / done states
// ---------------------------------------------------------------------------

TEST_F(AOTextBoxTest, TickReturnsFalseWhenInactive) {
    AOTextBox fresh;
    EXPECT_FALSE(fresh.tick(16));
}

TEST_F(AOTextBoxTest, TickReturnsFalseWhenDone) {
    box.start_message("Phoenix", "x", 0);
    // Tick until done
    for (int i = 0; i < 100; i++) box.tick(100);
    EXPECT_EQ(box.text_state(), AOTextBox::TextState::DONE);
    EXPECT_FALSE(box.tick(16));
}

// ---------------------------------------------------------------------------
// tick — ticking state advances text
// ---------------------------------------------------------------------------

TEST_F(AOTextBoxTest, TickWithEnoughTimeAdvancesTextAndReturnsTrue) {
    box.start_message("Phoenix", "Hello", 0);
    // BASE_TICK_MS=40, DEFAULT_SPEED=3, SPEED_MULT[3]=1.0 => delay=40ms
    // Provide enough time for at least one character advance.
    bool advanced = box.tick(50);
    EXPECT_TRUE(advanced);
}

TEST_F(AOTextBoxTest, TextStateTransitionsFromTickingToDoneAfterEnoughTicks) {
    box.start_message("Phoenix", "Hi", 0);
    EXPECT_EQ(box.text_state(), AOTextBox::TextState::TICKING);

    // "Hi" is 2 characters. At speed 3, delay=40ms per char.
    // Give enough time to finish both characters.
    box.tick(200);
    EXPECT_EQ(box.text_state(), AOTextBox::TextState::DONE);
}

// ---------------------------------------------------------------------------
// is_talking
// ---------------------------------------------------------------------------

TEST_F(AOTextBoxTest, IsTalkingReturnsFalseWhenInactive) {
    AOTextBox fresh;
    EXPECT_FALSE(fresh.is_talking());
}

TEST_F(AOTextBoxTest, IsTalkingReturnsFalseWhenDone) {
    box.start_message("Phoenix", "x", 0);
    for (int i = 0; i < 100; i++) box.tick(100);
    EXPECT_EQ(box.text_state(), AOTextBox::TextState::DONE);
    EXPECT_FALSE(box.is_talking());
}

TEST_F(AOTextBoxTest, IsTalkingReturnsTrueWhenTickingWithTalkingColor) {
    // Color index 0 is white with talking=true
    box.start_message("Phoenix", "Hello world", 0);
    EXPECT_EQ(box.text_state(), AOTextBox::TextState::TICKING);
    EXPECT_TRUE(box.is_talking());
}

TEST_F(AOTextBoxTest, IsTalkingReturnsFalseWhenTickingWithNonTalkingColor) {
    // Color index 3 is orange with talking=false
    box.start_message("Phoenix", "Hello world", 3);
    EXPECT_EQ(box.text_state(), AOTextBox::TextState::TICKING);
    EXPECT_FALSE(box.is_talking());
}

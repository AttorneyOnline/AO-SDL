#include "ao/game/AOMessagePlayer.h"

#include "ao/game/AOTextBox.h"
#include "asset/AssetLibrary.h"
#include "asset/MountManager.h"

#include <gtest/gtest.h>

namespace {

class AOMessagePlayerTest : public ::testing::Test {
  protected:
    void SetUp() override {
        // Load textbox so it has default text colors (prevents clamp crash)
        textbox.load(ao_assets);
    }

    MountManager mounts;
    AssetLibrary engine_assets{mounts, 0};
    AOAssetLibrary ao_assets{engine_assets};
    AOTextBox textbox;
    AOMessagePlayer player;

    ICMessage make_message(const std::string& character = "Phoenix", const std::string& message = "Hello!",
                           const std::string& side = "def") {
        ICMessage msg{};
        msg.character = character;
        msg.emote = "normal";
        msg.message = message;
        msg.showname = character;
        msg.side = side;
        msg.emote_mod = EmoteMod::IDLE;
        msg.desk_mod = DeskMod::CHAT;
        msg.text_color = 0;
        return msg;
    }
};

} // namespace

TEST_F(AOMessagePlayerTest, ReturnsICStateWithPosition) {
    auto result = player.play(make_message(), ao_assets, textbox, false);
    EXPECT_EQ(result.ic.position, "def");
}

TEST_F(AOMessagePlayerTest, DeskShownForDefPosition) {
    auto result = player.play(make_message("Phoenix", "Hi", "def"), ao_assets, textbox, false);
    EXPECT_TRUE(result.ic.show_desk);
}

TEST_F(AOMessagePlayerTest, DeskHiddenForJudPosition) {
    auto result = player.play(make_message("Judge", "Hi", "jud"), ao_assets, textbox, false);
    EXPECT_FALSE(result.ic.show_desk);
}

TEST_F(AOMessagePlayerTest, FlipStoredInState) {
    auto msg = make_message();
    msg.flip = true;
    auto result = player.play(msg, ao_assets, textbox, false);
    EXPECT_TRUE(result.ic.flip);
}

TEST_F(AOMessagePlayerTest, BlankMessageNotBlocking) {
    auto result = player.play(make_message("Phoenix", ""), ao_assets, textbox, false);
    EXPECT_FALSE(result.ic.preanim_blocking);
}

TEST_F(AOMessagePlayerTest, EmotePlayerStartsOnPlay) {
    auto result = player.play(make_message(), ao_assets, textbox, false);
    // Emote player should have been started (even if assets aren't loaded,
    // the state should advance from NONE)
    EXPECT_NE(result.ic.emote_player.state(), AOEmotePlayer::State::NONE);
}

TEST_F(AOMessagePlayerTest, ScreenshakeEffectReturned) {
    auto msg = make_message();
    msg.screenshake = true;
    auto result = player.play(msg, ao_assets, textbox, false);
    EXPECT_NE(std::find(result.effects.begin(), result.effects.end(), "screenshake"), result.effects.end());
}

TEST_F(AOMessagePlayerTest, FlashEffectReturned) {
    auto msg = make_message();
    msg.realization = true;
    auto result = player.play(msg, ao_assets, textbox, false);
    EXPECT_NE(std::find(result.effects.begin(), result.effects.end(), "flash"), result.effects.end());
}

TEST_F(AOMessagePlayerTest, NoEffectsForNormalMessage) {
    auto result = player.play(make_message(), ao_assets, textbox, false);
    EXPECT_TRUE(result.effects.empty());
}

TEST_F(AOMessagePlayerTest, PrevCharsVisibleInitializedToZero) {
    auto result = player.play(make_message(), ao_assets, textbox, false);
    EXPECT_EQ(result.ic.prev_chars_visible, 0);
}

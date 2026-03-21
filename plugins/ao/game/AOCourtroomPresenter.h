#pragma once

#include "AOBackground.h"
#include "AOEmotePlayer.h"
#include "AOTextBox.h"
#include "ICMessageQueue.h"
#include "ao/asset/AOAssetLibrary.h"
#include "asset/ImageAsset.h"
#include "ao/game/effects/FlashEffect.h"
#include "ao/game/effects/ScreenshakeEffect.h"
#include "ao/game/effects/ShaderEffect.h"
#include "game/IScenePresenter.h"

#include <atomic>
#include <memory>
#include <vector>

class ISceneEffect;

struct TickProfile {
    std::atomic<int> events_us{0};
    std::atomic<int> assets_us{0};
    std::atomic<int> animation_us{0};
    std::atomic<int> textbox_us{0};
    std::atomic<int> effects_us{0};
    std::atomic<int> compose_us{0};
    std::atomic<int> total_us{0};
};

class AOCourtroomPresenter : public IScenePresenter {
  public:
    AOCourtroomPresenter();
    void init() override;
    RenderState tick(uint64_t t) override;

    std::vector<ProfileEntry> tick_profile() const override {
        return {
            {"Events",    &profile_.events_us},
            {"Assets",    &profile_.assets_us},
            {"Animation", &profile_.animation_us},
            {"Textbox",   &profile_.textbox_us},
            {"Effects",   &profile_.effects_us},
            {"Compose",   &profile_.compose_us},
        };
    }

  private:
    void play_message(const ICMessage& msg);

    std::unique_ptr<AOAssetLibrary> ao_assets;
    AOBackground background;
    AOEmotePlayer emote_player;
    AOTextBox textbox;
    ICMessageQueue message_queue_;

    bool show_desk = true;
    bool current_flip = false;

    int evict_timer_ms = 0;

    // Scene effects
    ScreenshakeEffect screenshake_;
    FlashEffect flash_{BASE_W, BASE_H};
    ShaderEffect rainbow_{"shaders/rainbow", 5.0f, 5};
    ShaderEffect shatter_{"shaders/shatter", 4.0f, 5};
    ShaderEffect cube_{"shaders/cube", 0, 5};

    template <typename F> void for_each_effect(F&& fn) {
        fn(screenshake_); fn(flash_); fn(rainbow_); fn(shatter_); fn(cube_);
    }

    TickProfile profile_;

    // Base resolution for CPU-side rendering (textbox overlay, etc.)
    // GPU render texture resolution is controlled by DebugContext::internal_scale.
    static constexpr int BASE_W = 256;
    static constexpr int BASE_H = 192;
};

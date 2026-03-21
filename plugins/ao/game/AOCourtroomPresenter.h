#pragma once

#include "AOBackground.h"
#include "AOEmotePlayer.h"
#include "AOTextBox.h"
#include "ICMessageQueue.h"
#include "ao/asset/AOAssetLibrary.h"
#include "asset/ImageAsset.h"
#include "ao/game/effects/FlashEffect.h"
#include "ao/game/effects/ScreenshakeEffect.h"
#include "game/IScenePresenter.h"

#include <atomic>
#include <memory>
#include <vector>

class ISceneEffect;

struct TickProfile {
    std::atomic<int> events_us{0};
    std::atomic<int> animation_us{0};
    std::atomic<int> textbox_us{0};
    std::atomic<int> effects_us{0};
    std::atomic<int> compose_us{0};
    std::atomic<int> total_us{0};
};

class AOCourtroomPresenter : public IScenePresenter {
  public:
    AOCourtroomPresenter();
    RenderState tick(uint64_t t) override;

    std::vector<ProfileEntry> tick_profile() const override {
        return {
            {"Events",    &profile_.events_us},
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
    bool initialized = false;

    std::shared_ptr<ImageAsset> textbox_overlay;
    bool textbox_dirty = false;

    int evict_timer_ms = 0;

    // Scene effects
    ScreenshakeEffect screenshake_;
    FlashEffect flash_{VIEWPORT_W, VIEWPORT_H};
    std::vector<ISceneEffect*> effects_;

    TickProfile profile_;

    static constexpr int VIEWPORT_W = 256;
    static constexpr int VIEWPORT_H = 192;
};

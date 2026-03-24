#pragma once

#include "AOBackground.h"
#include "AOBlipPlayer.h"
#include "AOEmotePlayer.h"
#include "AOMusicPlayer.h"
#include "AOTextBox.h"
#include "ICMessageQueue.h"
#include "ao/asset/AOAssetLibrary.h"
#include "ao/game/effects/FlashEffect.h"
#include "ao/game/effects/ScreenshakeEffect.h"
#include "ao/game/effects/ShaderEffect.h"
#include "game/IScenePresenter.h"
#include "game/TickProfiler.h"

#include <atomic>
#include <memory>
#include <optional>
#include <string>
#include <vector>

class ISceneEffect;

class AOCourtroomPresenter : public IScenePresenter {
  public:
    AOCourtroomPresenter();
    void init() override;
    RenderState tick(uint64_t t) override;

    void set_courtroom_active(bool active) override {
        courtroom_active_.store(active, std::memory_order_release);
    }

    std::vector<ProfileEntry> tick_profile() const override {
        return profiler_.entries();
    }

  private:
    void play_message(const ICMessage& msg);

    std::unique_ptr<AOAssetLibrary> ao_assets;
    AOBackground background;
    AOEmotePlayer emote_player;
    AOTextBox textbox;
    ICMessageQueue message_queue_;

    // Per-IC-message state. Present while a message is active, cleared when
    // the message queue moves on.
    struct ActiveICState {
        bool flip = false;
        bool show_desk = true;
        bool preanim_blocking = false;
        std::string pending_showname;
        std::string pending_message;
        int pending_text_color = 0;
        bool pending_additive = false;
    };
    std::optional<ActiveICState> active_ic_;

    AOBlipPlayer blip_player_;
    AOMusicPlayer music_player_;
    int prev_chars_visible_ = 0;
    std::atomic<bool> courtroom_active_{false};

    int evict_timer_ms = 0;
    int theme_retry_ms_ = 0;
    float scene_time_s_ = 0; // monotonic time for shader effects

    // Scene effects
    ScreenshakeEffect screenshake_;
    FlashEffect flash_{BASE_W, BASE_H};
    ShaderEffect rainbow_{"shaders/rainbow", 5.0f, 5};
    ShaderEffect shatter_{"shaders/shatter", 4.0f, 5};
    ShaderEffect cube_{"shaders/cube", 0, 5};

    template <typename F>
    void for_each_effect(F&& fn) {
        fn(screenshake_);
        fn(flash_);
        fn(rainbow_);
        fn(shatter_);
        fn(cube_);
    }

    // Profiler sections (indices set in constructor)
    mutable TickProfiler profiler_;
    int prof_events_ = 0;
    int prof_assets_ = 0;
    int prof_animation_ = 0;
    int prof_textbox_ = 0;
    int prof_audio_ = 0;
    int prof_effects_ = 0;
    int prof_compose_ = 0;
    int prof_cache_ = 0;

    // Base resolution for CPU-side rendering (textbox overlay, etc.)
    // GPU render texture resolution is controlled by DebugContext::internal_scale.
    static constexpr int BASE_W = 256;
    static constexpr int BASE_H = 192;
};

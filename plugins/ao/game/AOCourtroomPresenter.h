#pragma once

#include "AOBackground.h"
#include "AOMessagePlayer.h"
#include "AOMusicPlayer.h"
#include "AOSceneCompositor.h"
#include "AOTextBox.h"
#include "ActiveICState.h"
#include "ICMessageQueue.h"
#include "ao/asset/AOAssetLibrary.h"
#include "ao/game/effects/FlashEffect.h"
#include "ao/game/effects/ScreenshakeEffect.h"
#include "ao/game/effects/ShaderEffect.h"
#include "ao/game/effects/SlideEffect.h"
#include "game/IScenePresenter.h"
#include "game/TickProfiler.h"

#include <array>
#include <atomic>
#include <memory>
#include <optional>
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

    void set_local_player(int char_id, bool slide_enabled) override {
        own_char_id_ = char_id;
        own_slide_enabled_ = slide_enabled;
    }

    std::vector<ProfileEntry> tick_profile() const override {
        return profiler_.entries();
    }

  private:
    void play_message(const ICMessage& msg);

    std::unique_ptr<AOAssetLibrary> ao_assets;
    AOBackground background;
    AOTextBox textbox;
    ICMessageQueue message_queue_;

    std::optional<ActiveICState> active_ic_;
    std::optional<ActiveICState> departing_ic_; // held during slide transitions

    AOSceneCompositor compositor_;
    AOMessagePlayer message_player_;
    AOMusicPlayer music_player_;
    std::atomic<bool> courtroom_active_{false};
    std::atomic<int> own_char_id_{-1};
    std::atomic<bool> own_slide_enabled_{false};

    int evict_timer_ms = 0;
    float scene_time_s_ = 0; // monotonic time for shader effects

    // Scene effects — keyed by name so AOMessagePlayer can trigger by string.
    // Slide is separate because it has a different lifecycle (not stopped on new message).
    ScreenshakeEffect screenshake_;
    FlashEffect flash_{BASE_W, BASE_H};
    ShaderEffect rainbow_{"shaders/rainbow", 5.0f, 5};
    ShaderEffect shatter_{"shaders/shatter", 4.0f, 5};
    ShaderEffect cube_{"shaders/cube", 0, 5};
    SlideEffect slide_effect_;

    struct NamedEffect {
        const char* name;
        ISceneEffect* effect;
    };
    std::array<NamedEffect, 5> message_effects_ = {{
        {"screenshake", &screenshake_},
        {"flash", &flash_},
        {"rainbow", &rainbow_},
        {"shatter", &shatter_},
        {"cube", &cube_},
    }};

    template <typename F>
    void for_each_effect(F&& fn) {
        for (auto& [name, effect] : message_effects_)
            fn(*effect);
        fn(slide_effect_);
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

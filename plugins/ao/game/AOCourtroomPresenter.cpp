#include "ao/game/AOCourtroomPresenter.h"

#include "ao/event/ICMessageEvent.h"
#include "asset/MediaManager.h"
#include "event/BackgroundEvent.h"
#include "event/EventManager.h"
#include "render/Layer.h"
#include "render/RenderState.h"
#include "utils/Log.h"

#include <cstring>
#include <random>

void AOCourtroomPresenter::trigger_screenshake() {
    // 300ms total, 20ms per jolt = 15 keyframes + final rest frame
    // Max deviation: 7/192 in NDC (base viewport height = 192px)
    constexpr int DURATION_MS = 300;
    constexpr int JOLT_MS = 20;
    constexpr float MAX_DEV = 7.0f / 192.0f * 2.0f; // NDC range is [-1, 1] = 2 units

    shake_anim_.clear_keyframes();
    shake_anim_.set_easing(Easing::LINEAR);
    shake_anim_.set_looping(false);

    thread_local std::mt19937 rng(std::random_device{}());
    std::uniform_real_distribution<float> dist(-MAX_DEV, MAX_DEV);

    int t = 0;
    while (t < DURATION_MS) {
        shake_anim_.add_keyframe({t, {dist(rng), dist(rng)}, {1, 1}, 0});
        t += JOLT_MS;
    }

    // Final keyframe: return to origin
    shake_anim_.add_keyframe({DURATION_MS, {0, 0}, {1, 1}, 0});

    shake_anim_.play();
}

RenderState AOCourtroomPresenter::tick(uint64_t t) {
    // t is real elapsed time in ms from the game thread
    int delta_ms = static_cast<int>(t);
    if (delta_ms <= 0)
        delta_ms = 16;
    if (delta_ms > 200)
        delta_ms = 200; // clamp to avoid huge jumps

    // Initialize AOAssetLibrary on first tick
    if (!initialized) {
        auto& engine = MediaManager::instance().assets();

        // Pick the best available theme
        // TODO: make theme configurable via settings
        std::string theme = "default";
        if (engine.config("themes/AceAttorney DS/courtroom_fonts.ini")) {
            theme = "AceAttorney DS";
        }

        ao_assets = std::make_unique<AOAssetLibrary>(engine, theme);
        textbox.load(*ao_assets);
        initialized = true;
        Log::log_print(DEBUG, "AOCourtroomPresenter: using theme '%s'", theme.c_str());
    }

    // ---- Drain background events ----
    auto& bg_ch = EventManager::instance().get_channel<BackgroundEvent>();
    while (auto ev = bg_ch.get_event()) {
        background.set(ev->get_background(), ev->get_position().empty() ? background.position() : ev->get_position());
    }

    // ---- Drain IC message events ----
    auto& ic_ch = EventManager::instance().get_channel<ICMessageEvent>();
    while (auto ev = ic_ch.get_event()) {
        if (!ev->get_side().empty()) {
            background.set_position(ev->get_side());
        }

        DeskMod dm = ev->get_desk_mod();
        show_desk = (dm == DeskMod::SHOW || dm == DeskMod::EMOTE_ONLY || dm == DeskMod::EMOTE_ONLY_EX);
        current_flip = ev->get_flip();

        emote_player.start(*ao_assets, ev->get_character(), ev->get_emote(), ev->get_pre_emote(), ev->get_emote_mod());

        textbox.start_message(ev->get_showname(), ev->get_message(), ev->get_text_color());
        textbox_dirty = true;

        if (ev->get_screenshake())
            pending_screenshake_ = true;
    }

    // ---- Update components ----
    background.reload_if_needed(*ao_assets);
    emote_player.tick(delta_ms);

    if (textbox.tick(delta_ms)) {
        textbox_dirty = true;
    }

    // Text finished scrolling → transition character to idle animation
    if (textbox.text_state() == AOTextBox::TextState::DONE && emote_player.state() == AOEmotePlayer::State::TALKING) {
        emote_player.transition_to_idle();
    }

    // ---- Screenshake ----
    if (pending_screenshake_) {
        trigger_screenshake();
        pending_screenshake_ = false;
    }
    shake_anim_.tick(delta_ms);

    // ---- Render textbox overlay when text changes ----
    if (textbox_dirty && textbox.text_state() != AOTextBox::TextState::INACTIVE) {
        std::vector<uint8_t> pixels(VIEWPORT_W * VIEWPORT_H * 4, 0);
        textbox.render(VIEWPORT_W, VIEWPORT_H, pixels.data());

        if (!textbox_overlay) {
            ImageFrame frame;
            frame.width = VIEWPORT_W;
            frame.height = VIEWPORT_H;
            frame.duration_ms = 0;
            frame.pixels = std::move(pixels);
            textbox_overlay =
                std::make_shared<ImageAsset>("_textbox_overlay", "raw", std::vector<ImageFrame>{std::move(frame)});
        }
        else {
            textbox_overlay->update_frame(0, std::move(pixels));
        }
        textbox_dirty = false;
    }

    // Periodic cache eviction (~every 30 seconds)
    evict_timer_ms += delta_ms;
    if (evict_timer_ms >= 30000) {
        evict_timer_ms = 0;
        ao_assets->engine_assets().evict_unused();
    }

    // ---- Assemble RenderState ----
    RenderState state;
    LayerGroup scene;

    if (background.bg_asset()) {
        scene.add_layer(0, Layer(background.bg_asset(), 0, 0));
    }

    if (emote_player.has_frame()) {
        scene.add_layer(5, Layer(emote_player.asset(), emote_player.current_frame_index(), 5));
    }

    if (background.desk_asset() && show_desk) {
        scene.add_layer(10, Layer(background.desk_asset(), 0, 10));
    }

    if (textbox_overlay && textbox.text_state() != AOTextBox::TextState::INACTIVE) {
        scene.add_layer(20, Layer(textbox_overlay, 0, 20));
    }

    // Apply screenshake to the entire scene group
    if (shake_anim_.is_playing() || !shake_anim_.is_finished()) {
        shake_anim_.apply(scene.transform());
    }

    state.add_layer_group(0, scene);
    return state;
}

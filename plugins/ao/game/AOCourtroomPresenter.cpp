#include "ao/game/AOCourtroomPresenter.h"

#include "ao/event/ICMessageEvent.h"
#include "asset/MediaManager.h"
#include "event/BackgroundEvent.h"
#include "event/EventManager.h"
#include "render/Layer.h"
#include "render/RenderState.h"
#include "utils/Log.h"

#include <cstring>

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

    state.add_layer_group(0, scene);
    return state;
}

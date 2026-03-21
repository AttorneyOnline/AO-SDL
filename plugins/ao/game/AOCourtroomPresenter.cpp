#include "ao/game/AOCourtroomPresenter.h"

#include "ao/event/ICLogEvent.h"
#include "ao/event/ICMessageEvent.h"
#include "asset/MediaManager.h"
#include "asset/MountManager.h"
#include "asset/ShaderAsset.h"
#include "event/BackgroundEvent.h"
#include "event/EventManager.h"
#include "render/Layer.h"
#include "render/RenderState.h"
#include "utils/Log.h"

#include <chrono>
#include <cstring>

using Clock = std::chrono::steady_clock;

/// Provides text color uniforms (u_text_r/g/b) to the text shader.
class TextColorProvider : public ShaderUniformProvider {
  public:
    TextColorProvider(float r, float g, float b) : r_(r), g_(g), b_(b) {}
    std::unordered_map<std::string, UniformValue> get_uniforms() const override {
        return {{"u_text_r", r_}, {"u_text_g", g_}, {"u_text_b", b_}};
    }
  private:
    float r_, g_, b_;
};

static int us_since(Clock::time_point start) {
    return static_cast<int>(std::chrono::duration_cast<std::chrono::microseconds>(Clock::now() - start).count());
}

AOCourtroomPresenter::AOCourtroomPresenter() {

    // Prefetch assets for queued messages so they're cache-warm when played
    message_queue_.set_prefetch([this](const ICMessage& msg) {
        if (!ao_assets) return;
        // Trigger HTTP downloads for missing assets (no-op if local)
        ao_assets->prefetch_character(msg.character, msg.emote, msg.pre_emote);
        // Also try loading locally cached assets into the decode cache
        ao_assets->character_emote(msg.character, msg.emote, "(a)");
        ao_assets->character_emote(msg.character, msg.emote, "(b)");
        if (!msg.pre_emote.empty())
            ao_assets->character_emote(msg.character, msg.pre_emote, "");
    });
}

void AOCourtroomPresenter::init() {
    auto& engine = MediaManager::instance().assets();

    std::string theme = "default";
    if (engine.config("themes/AceAttorney DS/courtroom_fonts.ini")) {
        theme = "AceAttorney DS";
    }

    ao_assets = std::make_unique<AOAssetLibrary>(engine, theme);
    textbox.load(*ao_assets);
    Log::log_print(DEBUG, "AOCourtroomPresenter: using theme '%s'", theme.c_str());
}

void AOCourtroomPresenter::play_message(const ICMessage& msg) {
    // Stop all active effects before starting a new message
    for_each_effect([](auto& e) { e.stop(); });

    if (!msg.side.empty())
        background.set_position(msg.side);

    show_desk = (msg.desk_mod == DeskMod::SHOW || msg.desk_mod == DeskMod::EMOTE_ONLY ||
                 msg.desk_mod == DeskMod::EMOTE_ONLY_EX);
    current_flip = msg.flip;

    // Prefetch character assets via HTTP
    ao_assets->prefetch_character(msg.character, msg.emote, msg.pre_emote, 2);

    // Resolve showname: prefer the one from the message, fall back to char.ini
    std::string showname = msg.showname;
    if (showname.empty()) {
        auto sheet = ao_assets->character_sheet(msg.character);
        showname = sheet ? sheet->showname() : msg.character;
    }

    textbox.start_message(showname, msg.message, msg.text_color, msg.additive);

    // Blank messages skip pre-anim and go straight to idle
    if (textbox.text_state() == AOTextBox::TextState::INACTIVE) {
        emote_player.start(*ao_assets, msg.character, msg.emote, "", EmoteMod::IDLE);
        emote_player.transition_to_idle();
    } else {
        emote_player.start(*ao_assets, msg.character, msg.emote, msg.pre_emote, msg.emote_mod);
    }

    if (msg.screenshake)
        screenshake_.trigger();
    if (msg.realization)
        flash_.trigger();
    if (msg.message.find("rainbow") != std::string::npos)
        rainbow_.trigger();
    if (msg.message.find("glass") != std::string::npos)
        shatter_.trigger();
    if (msg.message.find("cube") != std::string::npos)
        cube_.trigger();

    EventManager::instance().get_channel<ICLogEvent>().publish(
        ICLogEvent(showname, msg.message, msg.text_color));
}

RenderState AOCourtroomPresenter::tick(uint64_t t) {
    auto tick_start = Clock::now();

    int delta_ms = static_cast<int>(t);
    if (delta_ms <= 0)
        delta_ms = 16;
    if (delta_ms > 200)
        delta_ms = 200;

    // ---- Drain events into queue ----
    auto events_start = Clock::now();

    auto& bg_ch = EventManager::instance().get_channel<BackgroundEvent>();
    while (auto ev = bg_ch.get_event()) {
        background.set(ev->get_background(), ev->get_position().empty() ? background.position() : ev->get_position());

        // Area change: clear IC state so only the background shows
        textbox.start_message("", "", 0);
        message_queue_.clear();
        emote_player.stop();
        for_each_effect([](auto& e) { e.stop(); });
    }

    auto& ic_ch = EventManager::instance().get_channel<ICMessageEvent>();
    while (auto ev = ic_ch.get_event()) {
        message_queue_.enqueue(ICMessage::from_event(*ev));
    }

    // Advance queue — dequeue next message when current one finishes
    bool text_done = textbox.text_state() == AOTextBox::TextState::DONE ||
                     textbox.text_state() == AOTextBox::TextState::INACTIVE;
    message_queue_.tick(delta_ms, text_done);

    if (auto msg = message_queue_.next()) {
        play_message(*msg);
    }

    profile_.events_us.store(us_since(events_start), std::memory_order_relaxed);

    // ---- Assets (HTTP retries, sync fetches) ----
    auto assets_start = Clock::now();

    background.reload_if_needed(*ao_assets);
    emote_player.retry_load(*ao_assets);

    profile_.assets_us.store(us_since(assets_start), std::memory_order_relaxed);

    // ---- Animation ----
    auto anim_start = Clock::now();

    emote_player.tick(delta_ms);

    if (textbox.text_state() == AOTextBox::TextState::DONE && emote_player.state() == AOEmotePlayer::State::TALKING) {
        emote_player.transition_to_idle();
    }

    profile_.animation_us.store(us_since(anim_start), std::memory_order_relaxed);

    // ---- Textbox ----
    auto textbox_start = Clock::now();

    textbox.tick(delta_ms);

    profile_.textbox_us.store(us_since(textbox_start), std::memory_order_relaxed);

    // ---- Effects ----
    auto effects_start = Clock::now();

    for_each_effect([&](auto& e) { e.tick(delta_ms); });

    profile_.effects_us.store(us_since(effects_start), std::memory_order_relaxed);

    // ---- Compose RenderState ----
    auto compose_start = Clock::now();

    evict_timer_ms += delta_ms;
    if (evict_timer_ms >= 30000) {
        evict_timer_ms = 0;
        ao_assets->engine_assets().evict();
        // Flush raw HTTP bytes — anything decoded is in AssetCache,
        // anything still raw after 30s is probably not needed.
        MediaManager::instance().mounts_ref().release_all_http();
    }

    RenderState state;
    LayerGroup scene;

    if (background.bg_asset()) {
        scene.add_layer(0, Layer(background.bg_asset(), 0, 0));
    }

    if (emote_player.has_frame()) {
        Layer char_layer(emote_player.asset(), emote_player.current_frame_index(), 5);
        // Preserve sprite aspect ratio: lock height to viewport, scale width proportionally.
        float sprite_aspect = (float)emote_player.asset()->width() / (float)emote_player.asset()->height();
        float viewport_aspect = (float)BASE_W / (float)BASE_H;
        char_layer.transform().scale({sprite_aspect / viewport_aspect, 1.0f});
        scene.add_layer(5, std::move(char_layer));
    }

    if (background.desk_asset() && show_desk) {
        scene.add_layer(10, Layer(background.desk_asset(), 0, 10));
    }

    if (textbox.text_state() != AOTextBox::TextState::INACTIVE) {
        // Chatbox background — positioned via transform at the chatbox rect
        auto chatbox_bg = textbox.chatbox_background();
        if (chatbox_bg && chatbox_bg->frame_count() > 0) {
            const auto& rect = textbox.chatbox_position();
            float ndc_w = (float)chatbox_bg->width() / BASE_W * 2.0f;
            float ndc_h = (float)chatbox_bg->height() / BASE_H * 2.0f;
            float ndc_x = ((float)rect.x / BASE_W) * 2.0f - 1.0f + ndc_w * 0.5f;
            float ndc_y = 1.0f - ((float)rect.y / BASE_H) * 2.0f - ndc_h * 0.5f;

            Layer bg_layer(chatbox_bg, 0, 20);
            bg_layer.transform().scale({ndc_w * 0.5f, ndc_h * 0.5f});
            bg_layer.transform().translate({ndc_x, ndc_y});
            scene.add_layer(20, std::move(bg_layer));
        }

        // GPU text mesh
        auto atlas = textbox.message_atlas();
        auto mesh = textbox.message_mesh();
        auto shader = textbox.text_shader();
        if (atlas && mesh && mesh->index_count() > 0 && shader) {
            float r, g, b;
            textbox.message_color_rgb(r, g, b);

            auto provider = std::make_shared<TextColorProvider>(r, g, b);
            shader->set_uniform_provider(provider);

            Layer text_layer(atlas, 0, 21);
            text_layer.set_mesh(mesh);
            text_layer.set_shader(shader);
            scene.add_layer(21, std::move(text_layer));
        }
    }

    // Nameplate: rendered once, positioned and scaled via GPU transform
    auto nameplate = textbox.get_nameplate();
    if (nameplate && textbox.text_state() != AOTextBox::TextState::INACTIVE) {
        auto nl = textbox.nameplate_layout();

        float ndc_w = (float)nameplate->width() * nl.scale / BASE_W * 2.0f;
        float ndc_h = (float)nameplate->height() / (float)BASE_H * 2.0f;
        float ndc_x = ((float)nl.x / BASE_W) * 2.0f - 1.0f + ndc_w * 0.5f;
        float ndc_y = 1.0f - ((float)nl.y / BASE_H) * 2.0f - ndc_h * 0.5f;

        Layer nameplate_layer(nameplate, 0, 25);
        nameplate_layer.transform().scale({ndc_w * 0.5f, ndc_h * 0.5f});
        nameplate_layer.transform().translate({ndc_x, ndc_y});
        scene.add_layer(25, std::move(nameplate_layer));
    }

    for_each_effect([&](auto& e) {
        if (e.is_active())
            e.apply(scene);
    });

    state.add_layer_group(0, scene);

    profile_.compose_us.store(us_since(compose_start), std::memory_order_relaxed);
    profile_.total_us.store(us_since(tick_start), std::memory_order_relaxed);

    return state;
}

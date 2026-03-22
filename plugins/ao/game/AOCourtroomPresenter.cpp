#include "ao/game/AOCourtroomPresenter.h"

#include "ao/event/ICLogEvent.h"
#include "ao/event/ICMessageEvent.h"
#include "asset/MediaManager.h"
#include "asset/MountManager.h"
#include "asset/ShaderAsset.h"
#include "event/BackgroundEvent.h"
#include "event/EventManager.h"
#include "event/PlaySFXEvent.h"
#include "render/Layer.h"
#include "render/RenderState.h"
#include "utils/Log.h"

#include <chrono>
#include <cstring>

/// Provides text color uniforms (u_text_r/g/b) to the text shader.
class TextColorProvider : public ShaderUniformProvider {
  public:
    TextColorProvider(float r, float g, float b) : r_(r), g_(g), b_(b) {
    }
    std::unordered_map<std::string, UniformValue> get_uniforms() const override {
        return {{"u_text_r", r_}, {"u_text_g", g_}, {"u_text_b", b_}};
    }

  private:
    float r_, g_, b_;
};

AOCourtroomPresenter::AOCourtroomPresenter() {
    // Register profiler sections
    prof_events_ = profiler_.add_section("Events");
    prof_assets_ = profiler_.add_section("Assets");
    prof_animation_ = profiler_.add_section("Animation");
    prof_textbox_ = profiler_.add_section("Textbox");
    prof_audio_ = profiler_.add_section("Audio");
    prof_effects_ = profiler_.add_section("Effects");
    prof_compose_ = profiler_.add_section("Compose");

    // Prefetch assets for queued messages so they're cache-warm when played
    message_queue_.set_prefetch([this](const ICMessage& msg) {
        if (!ao_assets)
            return;
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
    ao_assets->prefetch_theme();
    textbox.load(*ao_assets);
    Log::log_print(DEBUG, "AOCourtroomPresenter: using theme '%s'", theme.c_str());
}

void AOCourtroomPresenter::play_message(const ICMessage& msg) {
    // Stop all active effects before starting a new message
    for_each_effect([](auto& e) { e.stop(); });

    if (!msg.side.empty())
        background.set_position(msg.side);

    if (msg.desk_mod == DeskMod::CHAT) {
        // Default: desk shown for def/pro/wit, hidden for other positions
        show_desk = (msg.side == "def" || msg.side == "pro" || msg.side == "wit");
    }
    else {
        show_desk = (msg.desk_mod == DeskMod::SHOW || msg.desk_mod == DeskMod::EMOTE_ONLY ||
                     msg.desk_mod == DeskMod::EMOTE_ONLY_EX);
    }
    current_flip = msg.flip;

    // Prefetch character assets via HTTP
    ao_assets->prefetch_character(msg.character, msg.emote, msg.pre_emote, 2);

    // Always load the character sheet so char.ini is promoted from the HTTP
    // raw cache into the asset cache (survives release_all_http eviction).
    auto sheet = ao_assets->character_sheet(msg.character);

    // Resolve showname: prefer the one from the message, fall back to char.ini
    std::string showname = msg.showname;
    if (showname.empty())
        showname = sheet ? sheet->showname() : msg.character;

    // Check if message text is blank (whitespace-only)
    bool blank = true;
    for (char c : msg.message) {
        if (c != ' ' && c != '\t' && c != '\n' && c != '\r') {
            blank = false;
            break;
        }
    }

    // Blank messages skip pre-anim and go straight to idle
    if (msg.message.empty() || blank) {
        textbox.start_message(showname, msg.message, msg.text_color, msg.additive);
        emote_player.start(*ao_assets, msg.character, msg.emote, "", EmoteMod::IDLE);
        emote_player.transition_to_idle();
        preanim_blocking_ = false;
    }
    else {
        emote_player.start(*ao_assets, msg.character, msg.emote, msg.pre_emote, msg.emote_mod);

        // Blocking preanim: defer textbox until preanim finishes
        if (emote_player.state() == AOEmotePlayer::State::PREANIM && !msg.immediate) {
            preanim_blocking_ = true;
            pending_showname_ = showname;
            pending_message_ = msg.message;
            pending_text_color_ = msg.text_color;
            pending_additive_ = msg.additive;
            // Keep textbox inactive during preanim
            textbox.start_message("", "", 0);
        }
        else {
            // Immediate (no-int-pre) or no preanim: start text right away
            preanim_blocking_ = false;
            textbox.start_message(showname, msg.message, msg.text_color, msg.additive);
        }
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

    // Resolve SFX: use packet sfx_name, fall back to char.ini emote SFX
    std::string sfx_name = msg.sfx_name;
    bool sfx_looping = msg.sfx_looping;
    if ((sfx_name.empty() || sfx_name == "0" || sfx_name == "1") && sheet) {
        auto* emote_entry = sheet->find_emote(msg.emote);
        if (emote_entry && !emote_entry->sfx_name.empty() && emote_entry->sfx_name != "0") {
            sfx_name = emote_entry->sfx_name;
            sfx_looping = emote_entry->sfx_looping;
        }
    }

    // Load and play SFX
    if (!sfx_name.empty() && sfx_name != "0" && sfx_name != "1") {
        auto sfx_asset = ao_assets->sound_effect(sfx_name);
        if (sfx_asset && courtroom_active_.load(std::memory_order_acquire)) {
            EventManager::instance().get_channel<PlaySFXEvent>().publish(PlaySFXEvent(sfx_asset, sfx_looping, 1.0f));
        }
    }

    // Initialize blip player for this character
    blip_player_.start(*ao_assets, msg.character);
    prev_chars_visible_ = 0;

    EventManager::instance().get_channel<ICLogEvent>().publish(ICLogEvent(showname, msg.message, msg.text_color));
}

RenderState AOCourtroomPresenter::tick(uint64_t t) {
    int delta_ms = static_cast<int>(t);
    if (delta_ms <= 0)
        delta_ms = 16;
    if (delta_ms > 200)
        delta_ms = 200;

    bool active = courtroom_active_.load(std::memory_order_acquire);

    // ---- Drain events into queue ----
    {
        auto _ = profiler_.scope(prof_events_);

        auto& bg_ch = EventManager::instance().get_channel<BackgroundEvent>();
        while (auto ev = bg_ch.get_event()) {
            background.set(ev->get_background(),
                           ev->get_position().empty() ? background.position() : ev->get_position());

            // Area change: clear IC state so only the background shows
            textbox.start_message("", "", 0);
            message_queue_.clear();
            emote_player.stop();
            preanim_blocking_ = false;
            for_each_effect([](auto& e) { e.stop(); });
        }

        auto& ic_ch = EventManager::instance().get_channel<ICMessageEvent>();
        while (auto ev = ic_ch.get_event()) {
            message_queue_.enqueue(ICMessage::from_event(*ev));
        }

        // Advance queue — dequeue next message when current one finishes.
        // Don't advance during a blocking preanim (textbox is INACTIVE but message isn't done).
        bool text_done = !preanim_blocking_ && (textbox.text_state() == AOTextBox::TextState::DONE ||
                                                textbox.text_state() == AOTextBox::TextState::INACTIVE);
        message_queue_.tick(delta_ms, text_done);

        if (auto msg = message_queue_.next()) {
            play_message(*msg);
        }
    }

    // ---- Assets (HTTP retries, sync fetches) ----
    {
        auto _ = profiler_.scope(prof_assets_);

        // Retry loading theme assets if they weren't available at init time.
        if (!textbox.loaded()) {
            textbox.load(*ao_assets);
        }

        background.reload_if_needed(*ao_assets);
        emote_player.retry_load(*ao_assets);
    }

    // ---- Animation ----
    {
        auto _ = profiler_.scope(prof_animation_);

        auto prev_emote_state = emote_player.state();
        emote_player.tick(delta_ms);

        // Blocking preanim just finished → start the deferred textbox
        if (preanim_blocking_ && prev_emote_state == AOEmotePlayer::State::PREANIM &&
            emote_player.state() == AOEmotePlayer::State::TALKING) {
            preanim_blocking_ = false;
            textbox.start_message(pending_showname_, pending_message_, pending_text_color_, pending_additive_);
            prev_chars_visible_ = 0;
        }

        if (textbox.text_state() == AOTextBox::TextState::DONE &&
            emote_player.state() == AOEmotePlayer::State::TALKING) {
            emote_player.transition_to_idle();
        }
    }

    // ---- Textbox ----
    int cur_chars;
    {
        auto _ = profiler_.scope(prof_textbox_);
        textbox.tick(delta_ms);
        cur_chars = textbox.chars_visible_count();
    }

    // ---- Audio (music + blips) ----
    {
        auto _ = profiler_.scope(prof_audio_);

        music_player_.tick(active);

        blip_player_.tick(*ao_assets, prev_chars_visible_, cur_chars, textbox.current_msg(), textbox.is_talking(),
                          active);
        prev_chars_visible_ = cur_chars;
    }

    // ---- Effects ----
    {
        auto _ = profiler_.scope(prof_effects_);
        for_each_effect([&](auto& e) { e.tick(delta_ms); });
    }

    // ---- Compose RenderState ----
    RenderState state;
    {
        auto _ = profiler_.scope(prof_compose_);

        evict_timer_ms += delta_ms;
        if (evict_timer_ms >= 30000) {
            evict_timer_ms = 0;
            ao_assets->engine_assets().evict();
            // Flush raw HTTP bytes — anything decoded is in AssetCache,
            // anything still raw after 30s is probably not needed.
            MediaManager::instance().mounts_ref().release_all_http();
        }

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
    }

    return state;
}

#include "ao/game/AOCourtroomPresenter.h"

#include "ao/event/ICMessageEvent.h"
#include "asset/MediaManager.h"
#include "asset/MountManager.h"
#include "event/BackgroundEvent.h"
#include "event/EventManager.h"
#include "render/RenderState.h"
#include "utils/Log.h"

AOCourtroomPresenter::AOCourtroomPresenter()
    : prof_events_(profiler_.add_section("Events")), prof_assets_(profiler_.add_section("Assets")),
      prof_animation_(profiler_.add_section("Animation")), prof_textbox_(profiler_.add_section("Textbox")),
      prof_audio_(profiler_.add_section("Audio")), prof_effects_(profiler_.add_section("Effects")),
      prof_compose_(profiler_.add_section("Compose")), prof_cache_(profiler_.add_section("Cache management")) {
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

    // Try to detect theme — if config isn't cached yet, use default.
    // Theme configs will be retried via textbox.load() on each tick.
    std::string theme = "default";
    engine.prefetch_config("themes/AceAttorney DS/courtroom_fonts.ini");
    if (engine.config("themes/AceAttorney DS/courtroom_fonts.ini")) {
        theme = "AceAttorney DS";
    }

    ao_assets = std::make_unique<AOAssetLibrary>(engine, theme);
    ao_assets->prefetch_theme();
    textbox.load(*ao_assets);
    Log::log_print(DEBUG, "AOCourtroomPresenter: using theme '%s'", theme.c_str());
}

void AOCourtroomPresenter::play_message(const ICMessage& msg) {
    // Stop message effects (not slide — it has its own lifecycle)
    for (auto& [name, effect] : message_effects_)
        effect->stop();

    // Slide if the sender requested it (msg.slide) and the position changed.
    // For our own messages, the flag may be force-enabled in the event drain
    // (own_slide_enabled_) since the server can strip it from the echo.
    std::string old_position = active_ic_ ? active_ic_->position : "";
    std::string new_position = msg.side;
    bool should_slide = false;

    if (active_ic_ && msg.slide && !old_position.empty() && !new_position.empty() && old_position != new_position &&
        msg.emote_mod != EmoteMod::ZOOM && msg.emote_mod != EmoteMod::PREANIM_ZOOM) {
        int duration = ao_assets->slide_duration_ms(background.background(), old_position, new_position);
        Log::log_print(DEBUG, "Slide check: bg='%s' %s→%s duration=%d", background.background().c_str(),
                       old_position.c_str(), new_position.c_str(), duration);

        if (duration > 0) {
            departing_ic_ = std::move(active_ic_);
            departing_ic_->bg_asset = background.bg_asset();
            departing_ic_->desk_asset = background.desk_asset();

            // Direction: if new origin > old origin, camera moves right (departing exits left).
            // Default to LEFT if origins aren't configured.
            auto from_origin = ao_assets->position_origin(background.background(), old_position);
            auto to_origin = ao_assets->position_origin(background.background(), new_position);
            auto dir = SlideEffect::Direction::LEFT;
            if (from_origin && to_origin && *to_origin < *from_origin)
                dir = SlideEffect::Direction::RIGHT;

            slide_effect_.configure(dir, duration);
            slide_effect_.trigger();
            should_slide = true;
            Log::log_print(DEBUG, "Slide: %s→%s (%dms, %s)", old_position.c_str(), new_position.c_str(), duration,
                           dir == SlideEffect::Direction::LEFT ? "left" : "right");
        }
    }

    if (!should_slide)
        departing_ic_.reset();

    if (!msg.side.empty())
        background.set_position(msg.side);

    bool active = courtroom_active_.load(std::memory_order_acquire);
    auto result = message_player_.play(msg, *ao_assets, textbox, active);

    // During a slide, suppress the textbox until the slide finishes.
    // Store the text data so it can be started after the slide completes.
    if (should_slide && !msg.message.empty()) {
        // Override any preanim blocking — slide takes priority and defers
        // the textbox until the slide finishes instead.
        result.ic.preanim_blocking = false;
        result.ic.slide_pending = true;
        result.ic.pending_showname = result.resolved_showname;
        result.ic.pending_message = msg.message;
        result.ic.pending_text_color = msg.text_color;
        result.ic.pending_additive = msg.additive;
        textbox.start_message("", "", 0, ao_assets->text_colors());
    }

    active_ic_.emplace(std::move(result.ic));

    // Fire scene effects by name
    for (const auto& name : result.effects) {
        for (auto& [ename, effect] : message_effects_) {
            if (name == ename) {
                effect->trigger();
                break;
            }
        }
    }
}

RenderState AOCourtroomPresenter::tick(uint64_t t) {
    int delta_ms = static_cast<int>(t);
    if (delta_ms <= 0)
        delta_ms = 16;
    if (delta_ms > 200)
        delta_ms = 200;

    scene_time_s_ += delta_ms / 1000.0f;

    bool active = courtroom_active_.load(std::memory_order_acquire);

    // ---- Drain events into queue ----
    {
        auto _ = profiler_.scope(prof_events_);

        auto& bg_ch = EventManager::instance().get_channel<BackgroundEvent>();
        while (auto ev = bg_ch.get_event()) {
            background.set(ev->get_background(),
                           ev->get_position().empty() ? background.position() : ev->get_position());

            // Area change: clear IC state so only the background shows
            textbox.start_message("", "", 0, ao_assets->text_colors());
            message_queue_.clear();
            active_ic_.reset();
            departing_ic_.reset();
            for_each_effect([](auto& e) { e.stop(); });
        }

        auto& ic_ch = EventManager::instance().get_channel<ICMessageEvent>();
        while (auto ev = ic_ch.get_event()) {
            auto msg = ICMessage::from_event(*ev);
            // Server may strip the slide field from the echo. If we have slide
            // enabled and this is our own message, force it on.
            if (!msg.slide && own_slide_enabled_ && msg.char_id == own_char_id_)
                msg.slide = true;
            message_queue_.enqueue(std::move(msg));
        }

        // Advance queue — dequeue next message when current one finishes.
        // Don't advance during a blocking preanim or slide (textbox is INACTIVE but message isn't done).
        bool preanim_blocking = active_ic_ && active_ic_->preanim_blocking;
        bool slide_blocking = active_ic_ && active_ic_->slide_pending;
        bool text_done = !preanim_blocking && !slide_blocking &&
                         (textbox.text_state() == AOTextBox::TextState::DONE ||
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
        if (active_ic_)
            active_ic_->emote_player.retry_load(*ao_assets);
        if (departing_ic_)
            departing_ic_->emote_player.retry_load(*ao_assets);
    }

    // ---- Animation ----
    {
        auto _ = profiler_.scope(prof_animation_);

        if (active_ic_) {
            auto& ic = *active_ic_;
            auto prev_emote_state = ic.emote_player.state();
            ic.emote_player.tick(delta_ms);

            // Blocking preanim just finished → start the deferred textbox
            if (ic.preanim_blocking && prev_emote_state == AOEmotePlayer::State::PREANIM &&
                ic.emote_player.state() == AOEmotePlayer::State::TALKING) {
                ic.preanim_blocking = false;
                textbox.start_message(ic.pending_showname, ic.pending_message, ic.pending_text_color,
                                      ao_assets->text_colors(), ic.pending_additive);
                ic.prev_chars_visible = 0;
            }

            if (textbox.text_state() == AOTextBox::TextState::DONE &&
                ic.emote_player.state() == AOEmotePlayer::State::TALKING) {
                ic.emote_player.transition_to_idle();
            }
        }

        if (departing_ic_)
            departing_ic_->emote_player.tick(delta_ms);
    }

    // ---- Textbox ----
    int cur_chars;
    bool text_advanced;
    {
        auto _ = profiler_.scope(prof_textbox_);
        auto tick_result = textbox.tick(delta_ms);
        if (tick_result.trigger_screenshake)
            screenshake_.trigger();
        if (tick_result.trigger_flash)
            flash_.trigger();
        cur_chars = textbox.chars_visible_count();
        text_advanced = tick_result.advanced;
    }

    // ---- Audio (music + blips) ----
    {
        auto _ = profiler_.scope(prof_audio_);

        music_player_.tick(active);

        if (active_ic_) {
            auto& ic = *active_ic_;
            // Use text_advanced instead of is_talking(): the textbox may have
            // already transitioned to DONE (e.g. single-char message) by the
            // time we get here, but blips should still play for the new chars.
            ic.blip_player.tick(*ao_assets, ic.prev_chars_visible, cur_chars, textbox.current_msg(), text_advanced,
                                active);
            ic.prev_chars_visible = cur_chars;
        }
    }

    // ---- Effects ----
    {
        auto _ = profiler_.scope(prof_effects_);
        for_each_effect([&](auto& e) { e.tick(delta_ms); });

        // Clean up departing scene after the pan finishes. needs_departing_scene()
        // is true during PRE_DELAY and SLIDING, false during POST_DELAY and INACTIVE.
        if (departing_ic_ && !slide_effect_.needs_departing_scene()) {
            departing_ic_.reset();
            if (active_ic_ && active_ic_->slide_pending) {
                active_ic_->slide_pending = false;
                // If the emote is still in preanim, chain into preanim blocking
                // instead of starting the textbox immediately.
                if (active_ic_->emote_player.state() == AOEmotePlayer::State::PREANIM) {
                    active_ic_->preanim_blocking = true;
                }
                else {
                    textbox.start_message(active_ic_->pending_showname, active_ic_->pending_message,
                                          active_ic_->pending_text_color, ao_assets->text_colors(),
                                          active_ic_->pending_additive);
                    active_ic_->prev_chars_visible = 0;
                }
            }
        }
    }

    // ---- Hint AssetCache Evictions ----
    {
        auto _ = profiler_.scope(prof_cache_);

        evict_timer_ms += delta_ms;
        if (evict_timer_ms >= 30000) {
            evict_timer_ms = 0;
            ao_assets->engine_assets().evict();
            // Flush raw HTTP bytes — anything decoded is in AssetCache,
            // anything still raw after 30s is probably not needed.
            MediaManager::instance().mounts_ref().release_all_http();
        }
    }

    // ---- Compose RenderState ----
    RenderState state;
    {
        auto _ = profiler_.scope(prof_compose_);

        state = compositor_.compose(background, active_ic_ ? &*active_ic_ : nullptr,
                                    departing_ic_ ? &*departing_ic_ : nullptr, textbox, scene_time_s_);

        if (departing_ic_) {
            // Slide: apply out/in transforms to separate layer groups
            auto* departing_group = state.get_mutable_layer_group(0);
            auto* arriving_group = state.get_mutable_layer_group(1);
            if (departing_group && slide_effect_.is_active())
                slide_effect_.apply_out(*departing_group);
            if (arriving_group && slide_effect_.is_active())
                slide_effect_.apply_in(*arriving_group);
            // Apply message effects to the arriving group
            if (arriving_group) {
                for (auto& [name, effect] : message_effects_) {
                    if (effect->is_active())
                        effect->apply(*arriving_group);
                }
            }
        }
        else {
            // Normal: apply all effects to the single layer group
            auto* scene = state.get_mutable_layer_group(0);
            if (scene) {
                for_each_effect([&](auto& e) {
                    if (e.is_active())
                        e.apply(*scene);
                });
            }
        }
    }

    return state;
}

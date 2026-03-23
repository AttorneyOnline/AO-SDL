#include "ao/game/AOTextBox.h"

#include "render/TextMeshBuilder.h"
#include "utils/ImageOps.h"
#include "utils/Log.h"
#include "utils/UTF8.h"

#include <algorithm>
#include <cstring>

constexpr double AOTextBox::SPEED_MULT[];

void AOTextBox::load(AOAssetLibrary& ao_assets) {
    engine_assets_ = &ao_assets.engine_assets();

    // Chatbox background images — variants for different showname widths
    chatbox_bg = ao_assets.theme_image("chat");
    if (!chatbox_bg)
        chatbox_bg = ao_assets.theme_image("chatbox");
    chatbox_med = ao_assets.theme_image("chatmed");
    chatbox_big = ao_assets.theme_image("chatbig");
    active_chatbox = chatbox_bg;

    // Layout from courtroom_design.ini.
    // Chatbox coordinates are in courtroom space; we need them relative to the
    // IC viewport. Subtract the viewport origin.
    AORect viewport = ao_assets.design_rect("viewport");
    chatbox_rect = ao_assets.design_rect("ao2_chatbox");
    chatbox_rect.x -= viewport.x;
    chatbox_rect.y -= viewport.y;

    // message and showname are already relative to the chatbox — no adjustment needed.
    message_rect = ao_assets.design_rect("message");
    showname_rect = ao_assets.design_rect("showname");

    // Showname extra width for chatmed/chatbig variants
    std::string extra_str = ao_assets.design_value("showname_extra_width");
    if (!extra_str.empty())
        showname_extra_width = std::atoi(extra_str.c_str());

    // Showname horizontal alignment (default: left)
    std::string align_str = ao_assets.design_value("showname_align");
    if (align_str == "center" || align_str == "justify")
        showname_align = Align::CENTER;
    else if (align_str == "right")
        showname_align = Align::RIGHT;
    else
        showname_align = Align::LEFT;

    // Text colors from chat_config.ini
    colors = ao_assets.text_colors();

    // Font
    AOFontSpec font = ao_assets.message_font_spec();
    auto font_data = ao_assets.find_font(font.name);
    // Fallback: try generic fonts
    if (!font_data)
        font_data = ao_assets.find_font("fot-modeminalargastd-r");

    if (font_data) {
        font_storage = std::move(*font_data);
        font_loaded = text_renderer.load_font_memory(font_storage.data(), font_storage.size(), font.size_px);
        text_renderer.set_sharp(font.sharp);
        Log::log_print(DEBUG, "AOTextBox: font='%s' %dpt (%dpx) sharp=%d", font.name.c_str(), font.size_pt,
                       font.size_px, font.sharp);
    }

    // System font fallback if no AO theme font was found
    if (!font_loaded) {
#ifdef __APPLE__
        font_loaded = text_renderer.load_font("/System/Library/Fonts/Helvetica.ttc", font.size_px);
#elif defined(_WIN32)
        font_loaded = text_renderer.load_font("C:\\Windows\\Fonts\\arial.ttf", font.size_px);
#else
        font_loaded = text_renderer.load_font("/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf", font.size_px);
#endif
        if (font_loaded) {
            text_renderer.set_sharp(font.sharp);
            Log::log_print(DEBUG, "AOTextBox: using system font fallback (%dpx)", font.size_px);
        }
    }

    if (!font_loaded) {
        Log::log_print(WARNING, "AOTextBox: no font loaded, text will not render");
    }

    // Showname font (may differ from message font)
    AOFontSpec sn_font = ao_assets.showname_font_spec();
    auto sn_font_data = ao_assets.find_font(sn_font.name);
    if (!sn_font_data)
        sn_font_data = font_data ? std::optional(std::vector<uint8_t>(font_storage)) : std::nullopt;

    if (sn_font_data) {
        showname_font_storage = std::move(*sn_font_data);
        showname_font_loaded = showname_renderer.load_font_memory(showname_font_storage.data(),
                                                                  showname_font_storage.size(), sn_font.size_px);
        showname_renderer.set_sharp(sn_font.sharp);
        Log::log_print(DEBUG, "AOTextBox: showname_font='%s' %dpt (%dpx) sharp=%d", sn_font.name.c_str(),
                       sn_font.size_pt, sn_font.size_px, sn_font.sharp);
    }
    if (!showname_font_loaded) {
#ifdef __APPLE__
        showname_font_loaded = showname_renderer.load_font("/System/Library/Fonts/Helvetica.ttc", sn_font.size_px);
#elif defined(_WIN32)
        showname_font_loaded = showname_renderer.load_font("C:\\Windows\\Fonts\\arial.ttf", sn_font.size_px);
#else
        showname_font_loaded =
            showname_renderer.load_font("/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf", sn_font.size_px);
#endif
        if (showname_font_loaded)
            showname_renderer.set_sharp(sn_font.sharp);
    }

    Log::log_print(DEBUG, "AOTextBox: chatbox=%d,%d,%dx%d message=%d,%d,%dx%d", chatbox_rect.x, chatbox_rect.y,
                   chatbox_rect.w, chatbox_rect.h, message_rect.x, message_rect.y, message_rect.w, message_rect.h);

    // GPU text: create glyph cache and load text shader
    if (font_loaded) {
        msg_glyph_cache_ = std::make_unique<GlyphCache>(text_renderer);
        msg_mesh_ = std::make_shared<MeshAsset>("_text_mesh", std::vector<MeshVertex>{}, std::vector<uint32_t>{});

        // Register the glyph atlas in the asset cache so it shows in the debug viewer
        engine_assets_->register_asset(msg_glyph_cache_->atlas_asset());

        text_shader_ = engine_assets_->shader("shaders/text");
        text_rainbow_shader_ = engine_assets_->shader("shaders/text_rainbow");
        if (text_shader_) {
            Log::log_print(DEBUG, "AOTextBox: text shader loaded for GPU rendering");
        }
    }
}

void AOTextBox::start_message(const std::string& showname, const std::string& message, int color_idx,
                              const std::vector<AOTextColorDef>& color_defs, bool additive) {
    current_showname = showname;

    if (additive) {
        previous_message += current_message;
    }
    else {
        previous_message.clear();
    }

    // Preprocess inline markup: escape sequences, speed modifiers, color markdown
    processed_ = AOTextProcessor::process(message, color_defs, color_idx);
    next_event_idx_ = 0;
    pause_remaining_ms_ = 0;

    current_message = processed_.display;
    base_color_idx_ = std::clamp(color_idx, 0, (int)colors.size() - 1);
    current_color_idx = base_color_idx_;
    chars_visible = 0;
    accumulated_ms = 0;
    current_speed = DEFAULT_SPEED;

    // Count UTF-8 characters in the display text (markup already stripped)
    total_chars = UTF8::length(current_message);

    // Don't show the textbox at all for empty/whitespace-only messages
    bool blank = true;
    for (char c : message) {
        if (c != ' ' && c != '\t' && c != '\n' && c != '\r') {
            blank = false;
            break;
        }
    }
    state = blank ? TextState::INACTIVE : (total_chars > 0 ? TextState::TICKING : TextState::DONE);
    last_chars_visible_ = -1; // force mesh rebuild on next tick

    // Clear GPU mesh when going inactive so stale text doesn't flash
    if (state == TextState::INACTIVE && msg_mesh_) {
        msg_mesh_->update({}, {});
    }

    // Pre-compute layout once — it doesn't change as characters appear
    cached_display_text_ = previous_message + current_message;
    cached_prev_chars_ = UTF8::length(previous_message);
    if (msg_glyph_cache_) {
        int wrap_w = message_rect.w > 0 ? message_rect.w : 256;
        cached_layout_ = text_renderer.compute_layout(cached_display_text_, wrap_w);

        // Apply text alignment (center/right/justify) from inline prefix
        if (processed_.alignment != TextAlignment::LEFT) {
            AOTextProcessor::apply_alignment(cached_layout_, processed_.alignment, wrap_w);
        }
    }
    else {
        cached_layout_.clear();
    }
}

void AOTextBox::message_color_rgb(float& r, float& g, float& b) const {
    if (current_color_idx >= 0 && current_color_idx < (int)colors.size()) {
        r = colors[current_color_idx].r / 255.0f;
        g = colors[current_color_idx].g / 255.0f;
        b = colors[current_color_idx].b / 255.0f;
    }
    else {
        r = g = b = 1.0f;
    }
}

bool AOTextBox::is_punctuation(char c) const {
    return c == '.' || c == ',' || c == '?' || c == '!' || c == ':' || c == ';';
}

int AOTextBox::current_tick_delay() const {
    int speed = std::clamp(current_speed, 0, 6);
    int delay = (int)(BASE_TICK_MS * SPEED_MULT[speed]);

    if (chars_visible > 0 && chars_visible <= (int)current_message.size()) {
        size_t byte_pos = UTF8::byte_offset(current_message, chars_visible - 1);
        if (byte_pos < current_message.size() && is_punctuation(current_message[byte_pos])) {
            delay *= PUNCTUATION_MULT;
        }
    }

    return std::max(delay, 1);
}

TickResult AOTextBox::tick(int delta_ms) {
    TickResult result;
    if (state != TextState::TICKING)
        return result;

    // Handle pause from \p escape sequence
    if (pause_remaining_ms_ > 0) {
        pause_remaining_ms_ -= delta_ms;
        return result;
    }

    accumulated_ms += delta_ms;
    int delay = current_tick_delay();

    while (accumulated_ms >= delay && chars_visible < total_chars) {
        accumulated_ms -= delay;
        chars_visible++;
        result.advanced = true;

        // Consume all events at or before the current character position
        while (next_event_idx_ < processed_.events.size() &&
               processed_.events[next_event_idx_].char_index <= chars_visible) {
            const auto& ev = processed_.events[next_event_idx_];
            switch (ev.type) {
            case TextEventType::COLOR_CHANGE:
                current_color_idx = std::clamp(ev.color_idx, 0, (int)colors.size() - 1);
                break;
            case TextEventType::SPEED_UP:
                current_speed = std::max(current_speed - 1, 1);
                break;
            case TextEventType::SPEED_DOWN:
                current_speed = std::min(current_speed + 1, 6);
                break;
            case TextEventType::PAUSE:
                pause_remaining_ms_ = 100;
                next_event_idx_++;
                goto done_advancing; // break out of both loops
            case TextEventType::SCREENSHAKE:
                result.trigger_screenshake = true;
                break;
            case TextEventType::FLASH:
                result.trigger_flash = true;
                break;
            }
            next_event_idx_++;
        }

        if (chars_visible >= total_chars) {
            state = TextState::DONE;
            break;
        }
        delay = current_tick_delay();
    }
done_advancing:

    // Rebuild GPU text mesh when characters advance (layout is cached from start_message)
    if (result.advanced && msg_glyph_cache_ && msg_mesh_ && chars_visible != last_chars_visible_) {
        last_chars_visible_ = chars_visible;

        int visible = cached_prev_chars_ + chars_visible;
        int scroll_y = text_renderer.compute_scroll_offset(cached_layout_, visible, message_rect.h);

        // Build per-character color array from processed events.
        // Walk events up to the current visible character count to determine
        // the color at each position.
        std::vector<TextMeshBuilder::CharColor> char_colors;
        int total_visible = cached_prev_chars_ + total_chars;
        char_colors.resize(total_visible);
        {
            // Use the original base color from the MS packet, not current_color_idx
            // which gets modified by inline COLOR_CHANGE events during tick.
            float br = 1.0f, bg = 1.0f, bb = 1.0f;
            int base_idx = base_color_idx_;
            br = colors[base_idx].r / 255.0f;
            bg = colors[base_idx].g / 255.0f;
            bb = colors[base_idx].b / 255.0f;

            // Fill previous message chars with base color
            for (int i = 0; i < cached_prev_chars_; i++)
                char_colors[i] = {br, bg, bb};

            // Walk events to compute color at each position in the current message
            float cr = br, cg = bg, cb = bb;
            size_t ev_idx = 0;
            for (int i = 0; i < total_chars; i++) {
                // Consume color events at this position
                while (ev_idx < processed_.events.size() && processed_.events[ev_idx].char_index <= i) {
                    if (processed_.events[ev_idx].type == TextEventType::COLOR_CHANGE) {
                        int ci = std::clamp(processed_.events[ev_idx].color_idx, 0, (int)colors.size() - 1);
                        cr = colors[ci].r / 255.0f;
                        cg = colors[ci].g / 255.0f;
                        cb = colors[ci].b / 255.0f;
                    }
                    ev_idx++;
                }
                char_colors[cached_prev_chars_ + i] = {cr, cg, cb};
            }
        }

        std::vector<MeshVertex> verts;
        std::vector<uint32_t> indices;
        TextMeshBuilder::build(*msg_glyph_cache_, cached_layout_, visible, chatbox_rect.x + message_rect.x,
                               chatbox_rect.y + message_rect.y, scroll_y, message_rect.h, 256, 192, verts, indices,
                               char_colors);
        msg_mesh_->update(std::move(verts), std::move(indices));
    }

    return result;
}

bool AOTextBox::is_talking() const {
    if (state != TextState::TICKING)
        return false;
    if (current_color_idx < 0 || current_color_idx >= (int)colors.size())
        return false;
    return colors[current_color_idx].talking;
}

std::shared_ptr<ImageAsset> AOTextBox::get_nameplate() {
    if (current_showname.empty() || !showname_font_loaded || !engine_assets_)
        return nullptr;

    // Fast path: same name as last time, return local cached ptr (no mutex)
    if (current_showname == cached_nameplate_name_ && cached_nameplate_)
        return cached_nameplate_;

    // Check the global asset cache (locks mutex)
    std::string cache_path = "_nameplate/" + current_showname;
    auto cached = std::dynamic_pointer_cast<ImageAsset>(engine_assets_->get_cached(cache_path));

    std::shared_ptr<ImageAsset> nameplate;

    if (cached) {
        nameplate = cached;
    }
    else {
        // Render text
        int text_w = showname_renderer.measure_width(current_showname);
        int text_h = showname_renderer.line_height();
        if (text_w <= 0 || text_h <= 0)
            return nullptr;

        std::vector<uint8_t> pixels(text_w * text_h * 4, 0);
        TextColor white = {255, 255, 255};
        showname_renderer.render(current_showname, (int)current_showname.size(), white, 0, 0, text_w, text_h, 0, 0,
                                 pixels.data());

        flip_vertical_rgba(pixels.data(), text_w, text_h);

        DecodedFrame frame;
        frame.width = text_w;
        frame.height = text_h;
        frame.duration_ms = 0;
        frame.pixels = std::move(pixels);

        nameplate = std::make_shared<ImageAsset>(cache_path, "gpu", std::vector<DecodedFrame>{std::move(frame)});
        engine_assets_->register_asset(nameplate);
    }

    cached_nameplate_ = nameplate;
    cached_nameplate_name_ = current_showname;

    // Compute layout — select chatbox variant based on showname width.
    // AO2 uses chat/chatmed/chatbig images with progressively wider name tabs.
    int text_w = nameplate->width();
    int text_h = nameplate->height();
    int extra_w = 0;
    if (text_w > showname_rect.w && chatbox_med) {
        extra_w = showname_extra_width;
        active_chatbox = chatbox_med;
    }
    if (text_w > showname_rect.w + showname_extra_width && chatbox_big) {
        extra_w = showname_extra_width * 2;
        active_chatbox = chatbox_big;
    }
    if (text_w <= showname_rect.w) {
        extra_w = 0;
        active_chatbox = chatbox_bg;
    }

    NameplateLayout nl;
    nl.w = showname_rect.w + extra_w;
    nl.h = showname_rect.h > 0 ? showname_rect.h : text_h;
    nl.scale = (text_w > nl.w && text_w > 0) ? (float)nl.w / text_w : 1.0f;
    int display_w = (int)(text_w * nl.scale);

    int x_offset = 0;
    if (showname_align == Align::CENTER)
        x_offset = (nl.w - display_w) / 2;
    else if (showname_align == Align::RIGHT)
        x_offset = nl.w - display_w;
    nl.x = chatbox_rect.x + showname_rect.x + x_offset;
    nl.y = chatbox_rect.y + showname_rect.y;
    cached_nameplate_layout_ = nl;

    return nameplate;
}

AOTextBox::NameplateLayout AOTextBox::nameplate_layout() const {
    return cached_nameplate_layout_;
}

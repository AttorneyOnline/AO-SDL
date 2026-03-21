#include "ao/game/AOTextBox.h"

#include "utils/BlendOps.h"
#include "utils/Log.h"
#include "utils/UTF8.h"

#include <algorithm>
#include <cstring>

constexpr double AOTextBox::SPEED_MULT[];

void AOTextBox::load(AOAssetLibrary& ao_assets) {
    engine_assets_ = &ao_assets.engine_assets();

    // Chatbox background image
    chatbox_bg = ao_assets.theme_image("chat");
    if (!chatbox_bg)
        chatbox_bg = ao_assets.theme_image("chatbox");

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
        showname_font_loaded = showname_renderer.load_font_memory(
            showname_font_storage.data(), showname_font_storage.size(), sn_font.size_px);
        showname_renderer.set_sharp(sn_font.sharp);
        Log::log_print(DEBUG, "AOTextBox: showname_font='%s' %dpt (%dpx) sharp=%d", sn_font.name.c_str(),
                       sn_font.size_pt, sn_font.size_px, sn_font.sharp);
    }

    Log::log_print(DEBUG, "AOTextBox: chatbox=%d,%d,%dx%d message=%d,%d,%dx%d", chatbox_rect.x, chatbox_rect.y,
                   chatbox_rect.w, chatbox_rect.h, message_rect.x, message_rect.y, message_rect.w, message_rect.h);
}

void AOTextBox::start_message(const std::string& showname, const std::string& message, int color_idx, bool additive) {
    current_showname = showname;

    if (additive) {
        previous_message += current_message;
    } else {
        previous_message.clear();
    }

    current_message = message;
    current_color_idx = std::clamp(color_idx, 0, (int)colors.size() - 1);
    chars_visible = 0;
    accumulated_ms = 0;
    current_speed = DEFAULT_SPEED;

    // Count UTF-8 characters
    total_chars = UTF8::length(message);

    // Don't show the textbox at all for empty/whitespace-only messages
    bool blank = true;
    for (char c : message) {
        if (c != ' ' && c != '\t' && c != '\n' && c != '\r') {
            blank = false;
            break;
        }
    }
    state = blank ? TextState::INACTIVE : (total_chars > 0 ? TextState::TICKING : TextState::DONE);
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

bool AOTextBox::tick(int delta_ms) {
    if (state != TextState::TICKING)
        return false;

    accumulated_ms += delta_ms;
    int delay = current_tick_delay();
    bool advanced = false;

    while (accumulated_ms >= delay && chars_visible < total_chars) {
        accumulated_ms -= delay;
        chars_visible++;
        advanced = true;

        if (chars_visible >= total_chars) {
            state = TextState::DONE;
            break;
        }
        delay = current_tick_delay();
    }

    return advanced;
}

bool AOTextBox::is_talking() const {
    if (state != TextState::TICKING)
        return false;
    if (current_color_idx < 0 || current_color_idx >= (int)colors.size())
        return false;
    return colors[current_color_idx].talking;
}

void AOTextBox::render(int viewport_w, int viewport_h, uint8_t* pixels) {
    if (!font_loaded)
        return;

    // Composite chatbox background (undo stbi flip by reading rows in reverse)
    if (chatbox_bg && chatbox_bg->frame_count() > 0) {
        const auto& frame = chatbox_bg->frame(0);
        for (int row = 0; row < frame.height && (chatbox_rect.y + row) < viewport_h; row++) {
            int src_row = frame.height - 1 - row;
            for (int col = 0; col < frame.width && (chatbox_rect.x + col) < viewport_w; col++) {
                int dx = chatbox_rect.x + col;
                int dy = chatbox_rect.y + row;
                if (dx < 0 || dy < 0)
                    continue;

                size_t src = ((size_t)src_row * frame.width + col) * 4;
                size_t dst = ((size_t)dy * viewport_w + dx) * 4;

                BlendOps::blend_over(&pixels[dst], &frame.pixels[src]);
            }
        }
    }

    if (state != TextState::INACTIVE && !current_message.empty()) {
        TextColor color = {colors[current_color_idx].r, colors[current_color_idx].g, colors[current_color_idx].b};

        std::string display_text = previous_message + current_message;
        int prev_chars = UTF8::length(previous_message);
        text_renderer.render(display_text, prev_chars + chars_visible, color, chatbox_rect.x + message_rect.x,
                             chatbox_rect.y + message_rect.y, viewport_w, viewport_h, message_rect.w, message_rect.h,
                             pixels);
    }

    // Flip vertically for GL (Y=0 at bottom)
    size_t row_bytes = (size_t)viewport_w * 4;
    std::vector<uint8_t> row_tmp(row_bytes);
    for (int y = 0; y < viewport_h / 2; y++) {
        uint8_t* top = pixels + y * row_bytes;
        uint8_t* bot = pixels + (viewport_h - 1 - y) * row_bytes;
        std::memcpy(row_tmp.data(), top, row_bytes);
        std::memcpy(top, bot, row_bytes);
        std::memcpy(bot, row_tmp.data(), row_bytes);
    }
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
    if (cached) {
        cached_nameplate_ = cached;
        cached_nameplate_name_ = current_showname;
        return cached;
    }

    // Render text and compute layout (only when cache miss)
    int text_w = showname_renderer.measure_width(current_showname);
    int text_h = showname_renderer.line_height();
    if (text_w <= 0 || text_h <= 0)
        return nullptr;

    std::vector<uint8_t> pixels(text_w * text_h * 4, 0);
    TextColor white = {255, 255, 255};
    showname_renderer.render(current_showname, (int)current_showname.size(), white, 0, 0, text_w, text_h, 0, 0,
                             pixels.data());

    // Flip vertically for GL
    size_t row_bytes = (size_t)text_w * 4;
    std::vector<uint8_t> row_tmp(row_bytes);
    for (int y = 0; y < text_h / 2; y++) {
        uint8_t* top = pixels.data() + y * row_bytes;
        uint8_t* bot = pixels.data() + (text_h - 1 - y) * row_bytes;
        std::memcpy(row_tmp.data(), top, row_bytes);
        std::memcpy(top, bot, row_bytes);
        std::memcpy(bot, row_tmp.data(), row_bytes);
    }

    ImageFrame frame;
    frame.width = text_w;
    frame.height = text_h;
    frame.duration_ms = 0;
    frame.pixels = std::move(pixels);

    auto nameplate = std::make_shared<ImageAsset>(cache_path, "gpu", std::vector<ImageFrame>{std::move(frame)});
    engine_assets_->register_asset(nameplate);
    cached_nameplate_ = nameplate;
    cached_nameplate_name_ = current_showname;

    // Cache the layout alongside the texture
    NameplateLayout nl;
    nl.w = showname_rect.w;
    nl.h = showname_rect.h > 0 ? showname_rect.h : text_h;
    nl.scale = (text_w > nl.w && text_w > 0) ? (float)nl.w / text_w : 1.0f;
    int display_w = (int)(text_w * nl.scale);

    int x_offset = 0;
    if (showname_align == Align::CENTER)
        x_offset = (showname_rect.w - display_w) / 2;
    else if (showname_align == Align::RIGHT)
        x_offset = showname_rect.w - display_w;
    nl.x = chatbox_rect.x + showname_rect.x + x_offset;
    nl.y = chatbox_rect.y + showname_rect.y;
    cached_nameplate_layout_ = nl;

    return nameplate;
}

AOTextBox::NameplateLayout AOTextBox::nameplate_layout() const {
    return cached_nameplate_layout_;
}


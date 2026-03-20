#include "ao/game/AOTextBox.h"

#include "utils/BlendOps.h"
#include "utils/Log.h"
#include "utils/UTF8.h"

#include <algorithm>
#include <cstring>

constexpr double AOTextBox::SPEED_MULT[];

void AOTextBox::load(AOAssetLibrary& ao_assets) {
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

    Log::log_print(DEBUG, "AOTextBox: chatbox=%d,%d,%dx%d message=%d,%d,%dx%d", chatbox_rect.x, chatbox_rect.y,
                   chatbox_rect.w, chatbox_rect.h, message_rect.x, message_rect.y, message_rect.w, message_rect.h);
}

void AOTextBox::start_message(const std::string& showname, const std::string& message, int color_idx) {
    current_showname = showname;
    current_message = message;
    current_color_idx = std::clamp(color_idx, 0, (int)colors.size() - 1);
    chars_visible = 0;
    accumulated_ms = 0;
    current_speed = DEFAULT_SPEED;

    // Count UTF-8 characters
    total_chars = UTF8::length(message);

    state = total_chars > 0 ? TextState::TICKING : TextState::DONE;
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

        if (!current_showname.empty()) {
            TextColor showname_color = {255, 255, 255};
            text_renderer.render(current_showname, (int)current_showname.size(), showname_color,
                                 chatbox_rect.x + showname_rect.x, chatbox_rect.y + showname_rect.y, viewport_w,
                                 viewport_h, showname_rect.w, 0, pixels);
        }

        text_renderer.render(current_message, chars_visible, color, chatbox_rect.x + message_rect.x,
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

#pragma once

#include "ao/asset/AOAssetLibrary.h"
#include "ao/game/AOTextProcessor.h"
#include "asset/ImageAsset.h"
#include "asset/MeshAsset.h"
#include "asset/ShaderAsset.h"
#include "render/GlyphCache.h"
#include "render/TextRenderer.h"

#include <cstdint>
#include <memory>
#include <string>
#include <vector>

/// Manages the AO2 courtroom chatbox: background image, showname, scrolling
/// text with per-character timing, text colors, and talking/idle signals.
///
/// All path resolution and config parsing is delegated to AOAssetLibrary.
class AOTextBox {
  public:
    enum class TextState { INACTIVE, TICKING, DONE };

    AOTextBox() = default;

    /// Load chatbox assets from AOAssetLibrary.
    void load(AOAssetLibrary& ao_assets);

    /// Returns true if essential assets (font + chatbox image) are loaded.
    bool loaded() const {
        return font_loaded && chatbox_bg;
    }

    void start_message(const std::string& showname, const std::string& message, int color_idx,
                       const std::vector<AOTextColorDef>& color_defs, bool additive = false);
    TickResult tick(int delta_ms);

    /// Render the showname into its own ImageAsset. Returns nullptr if empty.
    std::shared_ptr<ImageAsset> get_nameplate();

    /// Get the nameplate rect in viewport coordinates and the scale factor.
    struct NameplateLayout {
        int x, y, w, h;
        float scale;
    };
    NameplateLayout nameplate_layout() const;

    TextState text_state() const {
        return state;
    }
    bool is_talking() const;

    /// Number of UTF-8 characters currently visible in the message.
    int chars_visible_count() const {
        return chars_visible;
    }

    /// The current message text (for blip character checking).
    const std::string& current_msg() const {
        return current_message;
    }

    // --- GPU assets for the presenter ---

    /// Chatbox background image (positioned at chatbox_rect by presenter).
    /// Returns the active variant (chat/chatmed/chatbig) based on showname width.
    std::shared_ptr<ImageAsset> chatbox_background() const {
        return active_chatbox ? active_chatbox : chatbox_bg;
    }

    /// Message text mesh (rebuilt on tick when chars change).
    std::shared_ptr<MeshAsset> message_mesh() const {
        return msg_mesh_;
    }

    /// Glyph atlas texture for message text.
    std::shared_ptr<ImageAsset> message_atlas() const {
        return msg_glyph_cache_ ? msg_glyph_cache_->atlas_asset() : nullptr;
    }

    /// Text shader (normal or rainbow depending on current color).
    std::shared_ptr<ShaderAsset> text_shader() const {
        return is_rainbow() ? text_rainbow_shader_ : text_shader_;
    }

    /// Whether the current message uses rainbow text color.
    /// AO2 protocol: color index 6 is labeled "rainbow" in the client UI.
    bool is_rainbow() const {
        return current_color_idx == 6;
    }

    /// Current message color (for shader uniform).
    void message_color_rgb(float& r, float& g, float& b) const;

    /// Chatbox rect in viewport coordinates (for positioning the background layer).
    const AORect& chatbox_position() const {
        return chatbox_rect;
    }

  private:
    // Theme assets — chatbox variants for different showname widths
    std::shared_ptr<ImageAsset> chatbox_bg;     // default (shortest name tab)
    std::shared_ptr<ImageAsset> chatbox_med;    // medium name tab
    std::shared_ptr<ImageAsset> chatbox_big;    // large name tab
    std::shared_ptr<ImageAsset> active_chatbox; // currently selected variant
    int showname_extra_width = 24;
    TextRenderer text_renderer;
    TextRenderer showname_renderer;
    bool font_loaded = false;
    bool showname_font_loaded = false;

    // Layout from courtroom_design.ini
    AORect chatbox_rect = {0, 114, 256, 78};
    AORect message_rect = {10, 13, 242, 57};
    AORect showname_rect = {1, 0, 46, 15};
    enum class Align { LEFT, CENTER, RIGHT };
    Align showname_align = Align::LEFT;

    // Colors from chat_config.ini
    std::vector<AOTextColorDef> colors;

    // Current message state
    std::string current_showname;
    std::string current_message;  // display text (markup stripped)
    std::string previous_message; // for additive mode
    int current_color_idx = 0;
    int base_color_idx_ = 0; ///< Original color from MS packet (not modified by inline events).
    int chars_visible = 0;
    int total_chars = 0;
    TextState state = TextState::INACTIVE;

    // Preprocessed text events
    ProcessedText processed_;
    size_t next_event_idx_ = 0;
    int pause_remaining_ms_ = 0;

    // Tick timing
    static constexpr int BASE_TICK_MS = 40;
    static constexpr double SPEED_MULT[] = {0, 0.25, 0.65, 1.0, 1.25, 1.75, 2.25};
    static constexpr int DEFAULT_SPEED = 3;
    static constexpr int PUNCTUATION_MULT = 3;
    int accumulated_ms = 0;
    int current_speed = DEFAULT_SPEED;

    bool is_punctuation(char c) const;
    int current_tick_delay() const;

    // Nameplate: stored in the engine asset cache under "_nameplate/<showname>".
    // Local shared_ptr avoids hitting the cache mutex on every tick.
    AssetLibrary* engine_assets_ = nullptr;
    std::string cached_nameplate_name_;
    std::shared_ptr<ImageAsset> cached_nameplate_;
    NameplateLayout cached_nameplate_layout_{};

    // Persistent font data (TextRenderer needs the buffer alive)
    std::vector<uint8_t> font_storage;
    std::vector<uint8_t> showname_font_storage;

    // GPU text rendering
    std::unique_ptr<GlyphCache> msg_glyph_cache_;
    std::shared_ptr<MeshAsset> msg_mesh_;
    std::shared_ptr<ShaderAsset> text_shader_;
    std::shared_ptr<ShaderAsset> text_rainbow_shader_;
    int last_chars_visible_ = -1; // track when mesh needs rebuild

    // Cached layout (computed once in start_message, reused every tick)
    std::vector<TextRenderer::GlyphLayout> cached_layout_;
    std::string cached_display_text_;
    int cached_prev_chars_ = 0;
};

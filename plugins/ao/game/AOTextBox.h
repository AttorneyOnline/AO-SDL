#pragma once

#include "ao/asset/AOAssetLibrary.h"
#include "asset/ImageAsset.h"
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

    void start_message(const std::string& showname, const std::string& message, int color_idx,
                       bool additive = false);
    bool tick(int delta_ms);

    /// Render the showname into its own ImageAsset. Returns nullptr if empty.
    /// The asset is cached and only re-rendered when the name changes.
    std::shared_ptr<ImageAsset> get_nameplate();

    /// Get the nameplate rect in viewport coordinates and the scale factor
    /// needed to fit the rendered text within the box.
    struct NameplateLayout {
        int x, y, w, h;    // position and box size in viewport pixels
        float scale;        // horizontal scale to fit text in box (1.0 = fits naturally)
    };
    NameplateLayout nameplate_layout() const;

    TextState text_state() const {
        return state;
    }
    bool is_talking() const;

    /// Render chatbox into a viewport-sized RGBA pixel buffer (pre-cleared).
    void render(int viewport_w, int viewport_h, uint8_t* pixels);

  private:
    // Theme assets
    std::shared_ptr<ImageAsset> chatbox_bg;
    TextRenderer text_renderer;
    bool font_loaded = false;

    // Layout from courtroom_design.ini
    AORect chatbox_rect = {0, 114, 256, 78};
    AORect message_rect = {10, 13, 242, 57};
    AORect showname_rect = {1, 0, 46, 15};

    // Colors from chat_config.ini
    std::vector<AOTextColorDef> colors;

    // Current message state
    std::string current_showname;
    std::string current_message;
    std::string previous_message; // for additive mode
    int current_color_idx = 0;
    int chars_visible = 0;
    int total_chars = 0;
    TextState state = TextState::INACTIVE;

    // Tick timing
    static constexpr int BASE_TICK_MS = 40;
    static constexpr double SPEED_MULT[] = {0, 0.25, 0.65, 1.0, 1.25, 1.75, 2.25};
    static constexpr int DEFAULT_SPEED = 3;
    static constexpr int PUNCTUATION_MULT = 3;
    int accumulated_ms = 0;
    int current_speed = DEFAULT_SPEED;

    bool is_punctuation(char c) const;
    int current_tick_delay() const;

    // Nameplate cache
    std::string cached_nameplate_name_;
    std::shared_ptr<ImageAsset> cached_nameplate_;

    // Persistent font data (TextRenderer needs the buffer alive)
    std::vector<uint8_t> font_storage;
};

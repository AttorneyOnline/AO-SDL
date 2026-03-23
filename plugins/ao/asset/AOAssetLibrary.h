#pragma once

#include "ao/asset/AOCharacterSheet.h"
#include "asset/AssetLibrary.h"
#include "asset/ImageAsset.h"
#include "asset/SoundAsset.h"

#include <cstdint>
#include <memory>
#include <optional>
#include <string>
#include <vector>

/// Rect from courtroom_design.ini: x, y, width, height.
struct AORect {
    int x = 0, y = 0, w = 0, h = 0;
};

/// Font settings from courtroom_fonts.ini.
struct AOFontSpec {
    std::string name;  ///< Normalized filename stem (e.g. "igiari-cyrillic").
    int size_pt = 10;  ///< Point size from config.
    int size_px = 13;  ///< Pixel size (pt * 4/3, rounded).
    bool sharp = true; ///< No anti-aliasing (message_sharp=1).
};

/// Text color from chat_config.ini.
struct AOTextColorDef {
    uint8_t r = 247, g = 247, b = 247;
    bool talking = true;
};

/// The AO2-specific asset resolution layer.
///
/// Knows how to construct virtual paths for AO2 assets (characters, backgrounds,
/// themes, etc.) and drives the fallback chain logic from the legacy client.
/// All actual I/O is delegated to the underlying AssetLibrary.
///
/// This class owns ALL AO2 path conventions. Nothing outside ao/ should need to
/// know about char.ini paths, position names, theme directories, or VPath chains.
class AOAssetLibrary {
  public:
    explicit AOAssetLibrary(AssetLibrary& assets, const std::string& theme = "default");

    const std::string& theme() const {
        return active_theme;
    }

    // -------------------------------------------------------------------------
    // Character sprites
    // -------------------------------------------------------------------------

    /// Load a character emote with a prefix: "(a)", "(b)", or "" for preanim.
    std::shared_ptr<ImageAsset> character_emote(const std::string& character, const std::string& emote,
                                                const std::string& prefix);

    std::shared_ptr<ImageAsset> character_icon(const std::string& character);

    /// Load an emote button icon (emotions/button{N}_off).
    std::shared_ptr<ImageAsset> emote_icon(const std::string& character, int emote_index);

    /// Parse char.ini and return the character sheet.
    std::optional<AOCharacterSheet> character_sheet(const std::string& character);

    // -------------------------------------------------------------------------
    // Background
    // -------------------------------------------------------------------------

    /// Load the background image for a position (legacy name mapping + fallbacks).
    /// If @p no_default is true, skip the default background fallback.
    std::shared_ptr<ImageAsset> background(const std::string& name, const std::string& position,
                                           bool no_default = false);

    /// Load the desk overlay for a position.
    std::shared_ptr<ImageAsset> desk_overlay(const std::string& name, const std::string& position);

    /// Prefetch background images for a position (legacy + modern names).
    void prefetch_background(const std::string& name, const std::string& position, int priority = 3);

    // -------------------------------------------------------------------------
    // Theme / UI
    // -------------------------------------------------------------------------

    /// Load a theme image (e.g. "chat", "chatblank"). Active theme → "default".
    std::shared_ptr<ImageAsset> theme_image(const std::string& element);

    /// Load a theme config INI. Active theme → "default".
    std::optional<IniDocument> theme_config(const std::string& filename);

    /// Parse a rect from courtroom_design.ini.
    AORect design_rect(const std::string& key);

    /// Read a raw string value from courtroom_design.ini.
    std::string design_value(const std::string& key);

    /// Get the message font spec from courtroom_fonts.ini.
    AOFontSpec message_font_spec();

    /// Get the showname font spec from courtroom_fonts.ini.
    AOFontSpec showname_font_spec();

    /// Get text colors from chat_config.ini (indices 0-8).
    std::vector<AOTextColorDef> text_colors();

    /// Find a font file by normalized name. Searches all theme dirs + fonts/.
    std::optional<std::vector<uint8_t>> find_font(const std::string& normalized_name);

    // -------------------------------------------------------------------------
    // Audio
    // -------------------------------------------------------------------------

    /// Load a sound effect by name (from [SoundN] in char.ini).
    /// Probes: sounds/general/{name}, sounds/{name}.
    std::shared_ptr<SoundAsset> sound_effect(const std::string& sfx_name);

    /// Load a blip sound by name (from [Options] blips= in char.ini).
    /// Probes: sounds/blips/{name}.
    std::shared_ptr<SoundAsset> blip_sound(const std::string& blip_name);

    /// Load the blip for a character (reads char.ini blips field → calls blip_sound).
    std::shared_ptr<SoundAsset> character_blip(const std::string& character);

    AssetLibrary& engine_assets() {
        return assets;
    }

    /// Trigger background HTTP prefetch for a character's emote assets.
    /// No-op if assets are available locally or no HTTP mount is configured.
    void prefetch_character(const std::string& character, const std::string& emote, const std::string& pre_emote,
                            int priority = 1);

    /// Prefetch all emote icons and sprites for the player's own character.
    /// Uses HIGH priority since these are needed immediately.
    void prefetch_own_character(const std::string& character);

    /// Prefetch essential theme assets (chatbox images, etc.) via HTTP.
    void prefetch_theme();

    /// Prefetch an image using server-advertised extensions for the given AO
    /// asset type. Falls back to default image extensions if the server hasn't
    /// provided an extensions.json.
    /// asset_type: 0=charicon, 1=emote, 2=emotions, 3=background
    void prefetch_image(const std::string& path, int asset_type, int priority = 1);

  private:
    AssetLibrary& assets;
    std::string active_theme;

    std::optional<IniDocument> cached_design;
    std::optional<IniDocument> cached_fonts;
    std::optional<IniDocument> cached_chat_config;
    bool configs_loaded = false;
    void ensure_configs();

    static std::string bg_filename(const std::string& position);
    static std::string desk_filename(const std::string& position);
    static std::string normalize_font_name(const std::string& name);
};

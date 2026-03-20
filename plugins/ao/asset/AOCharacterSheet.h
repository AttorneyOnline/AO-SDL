#pragma once

#include "asset/AssetLibrary.h"

#include <optional>
#include <string>
#include <vector>

/// A single emote entry parsed from char.ini [Emotions].
struct AOEmoteEntry {
    std::string comment;   ///< Display name for the emote button.
    std::string pre_anim;  ///< Pre-animation name, or "-" for none.
    std::string anim_name; ///< Main animation name.
    int mod = 0;           ///< Emote modifier.
    int desk_mod = 0;      ///< Desk modifier.
    std::string sfx_name;  ///< SFX from [SoundN], empty if none.
    int sfx_delay = 0;     ///< SFX delay from [SoundT] (raw frame count).
    bool sfx_looping = false; ///< Whether the SFX loops.
};

/// Parsed representation of a character's char.ini.
///
/// Owns all the data from [Options], [Emotions], [SoundN], [SoundT], [Time],
/// and per-emote frame effect sections. Constructed from an IniDocument.
class AOCharacterSheet {
  public:
    /// Parse a char.ini for the given character name.
    /// Returns nullopt if the ini file was not found.
    static std::optional<AOCharacterSheet> load(AssetLibrary& assets, const std::string& character);

    const std::string& name() const { return name_; }
    const std::string& showname() const { return showname_; }
    const std::string& side() const { return side_; }
    const std::string& blips() const { return blips_; }
    const std::string& effects_folder() const { return effects_; }
    const std::string& chat_style() const { return chat_; }

    int emote_count() const { return (int)emotes_.size(); }
    const AOEmoteEntry& emote(int index) const { return emotes_.at(index); }
    const std::vector<AOEmoteEntry>& emotes() const { return emotes_; }

    /// Get the pre-animation duration override from [Time], or 0 if not set.
    int preanim_duration_ms(const std::string& pre_anim) const;

    /// Get frame-triggered SFX for an emote's animation.
    /// Returns map of frame_number → sfx_name, or empty if none.
    const std::vector<std::pair<int, std::string>>& frame_sfx(const std::string& anim_name) const;

    /// Get frame-triggered screenshake for an emote's animation.
    const std::vector<int>& frame_screenshake(const std::string& anim_name) const;

    /// Get frame-triggered realization for an emote's animation.
    const std::vector<int>& frame_realization(const std::string& anim_name) const;

  private:
    AOCharacterSheet() = default;

    void parse_options(const IniDocument& ini);
    void parse_emotes(const IniDocument& ini);
    void parse_sounds(const IniDocument& ini);
    void parse_frame_effects(const IniDocument& ini);
    void parse_time(const IniDocument& ini);

    static const std::unordered_map<std::string, std::string>*
    find_section(const IniDocument& ini, const std::string& name);

    std::string name_;
    std::string showname_;
    std::string side_ = "wit";
    std::string blips_ = "male";
    std::string effects_;
    std::string chat_ = "default";

    std::vector<AOEmoteEntry> emotes_;
    std::unordered_map<std::string, int> preanim_durations_;

    // Per-animation frame effects, keyed by animation name
    std::unordered_map<std::string, std::vector<std::pair<int, std::string>>> frame_sfx_;
    std::unordered_map<std::string, std::vector<int>> frame_screenshake_;
    std::unordered_map<std::string, std::vector<int>> frame_realization_;

    // Statics for returning empty results
    static const std::vector<std::pair<int, std::string>> empty_frame_sfx_;
    static const std::vector<int> empty_frame_ints_;
};

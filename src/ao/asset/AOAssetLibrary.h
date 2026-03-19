#pragma once

#include "asset/AssetLibrary.h"

#include <optional>
#include <string>
#include <vector>

enum class EmoteType {
    Idle,    // looping idle (b-emote)
    Preanim, // one-shot pre-animation (a-emote)
    Talking, // talking variant
};

enum class ShoutType {
    Objection,
    HoldIt,
    TakeThat,
    Custom,
};

// The AO2-specific asset resolution layer.
//
// Knows how to construct virtual paths for AO2 assets (characters, backgrounds,
// themes, etc.) and drives the fallback chain logic from the legacy client.
// All actual I/O is delegated to the underlying AssetLibrary.
//
// This class owns the AO2 conventions. Nothing outside ao/ should need to know
// about char.ini paths, position names, misc folders, or VPath chains.
class AOAssetLibrary {
  public:
    explicit AOAssetLibrary(AssetLibrary& assets);

    // -------------------------------------------------------------------------
    // Character
    // -------------------------------------------------------------------------

    std::optional<RawAsset> character_emote(
        const std::string& character,
        const std::string& emote,
        EmoteType type);

    std::optional<RawAsset> character_icon(const std::string& character);
    std::optional<IniDocument> character_config(const std::string& character);

    // -------------------------------------------------------------------------
    // Background
    // -------------------------------------------------------------------------

    std::optional<RawAsset> background(
        const std::string& name,
        const std::string& position);

    std::optional<RawAsset> background_overlay(
        const std::string& name,
        const std::string& position);

    std::optional<IniDocument> background_config(const std::string& name);

    // -------------------------------------------------------------------------
    // Theme / UI
    // -------------------------------------------------------------------------

    // Resolves through: subtheme/misc -> theme/misc -> misc -> subtheme -> theme -> default
    std::optional<RawAsset> theme_image(
        const std::string& element,
        const std::string& theme,
        const std::string& subtheme = "",
        const std::string& misc = "");

    std::optional<IniDocument> theme_config(
        const std::string& filename,
        const std::string& theme,
        const std::string& subtheme = "",
        const std::string& misc = "");

    // -------------------------------------------------------------------------
    // Shouts / Effects
    // -------------------------------------------------------------------------

    std::optional<RawAsset> shout(
        ShoutType type,
        const std::string& theme,
        const std::string& misc = "",
        const std::string& custom_name = "");

    std::optional<RawAsset> effect(
        const std::string& name,
        const std::string& theme,
        const std::string& misc = "");

    std::optional<RawAsset> effect_icon(
        const std::string& name,
        const std::string& theme,
        const std::string& misc = "");

    // -------------------------------------------------------------------------
    // Audio
    // -------------------------------------------------------------------------

    // Searched in: character folder -> misc folder -> sounds/general/
    std::optional<RawAsset> sfx(
        const std::string& name,
        const std::string& character = "",
        const std::string& misc = "");

    // Returns nullopt for remote URLs — check music_url() first.
    std::optional<RawAsset> music(const std::string& name);
    std::optional<std::string> music_url(const std::string& name);

    std::optional<RawAsset> blip(const std::string& name);

    // -------------------------------------------------------------------------
    // Evidence
    // -------------------------------------------------------------------------

    std::optional<RawAsset> evidence(const std::string& id);

    // -------------------------------------------------------------------------
    // Enumeration
    // -------------------------------------------------------------------------

    std::vector<std::string> character_list();
    std::vector<std::string> background_list();
    std::vector<std::string> music_list();

  private:
    AssetLibrary& assets;

    // Builds an ordered list of virtual paths to probe for a given asset,
    // implementing the AO2 fallback chain.
    std::vector<std::string> resolve_paths(
        const std::string& element,
        const std::string& theme,
        const std::string& subtheme,
        const std::string& misc);
};

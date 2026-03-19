#pragma once

#include "asset/AssetLibrary.h"
#include "asset/ImageAsset.h"

#include <memory>
#include <string>

/// Manages courtroom background and desk overlay loading using AO2 conventions.
///
/// Knows the AO2 position→filename mapping (def→defenseempty, etc.) and the
/// fallback chain (named bg → modern naming → default bg). Exposes loaded
/// ImageAssets for the presenter to reference in Layers.
class AOBackground {
  public:
    void set(const std::string& background, const std::string& position);
    void set_position(const std::string& position);
    void reload_if_needed(AssetLibrary& assets);

    const std::string& background() const { return bg_name; }
    const std::string& position() const { return pos; }

    /// Background asset, or null if not loaded.
    const std::shared_ptr<ImageAsset>& bg_asset() const { return bg; }

    /// Desk overlay asset, or null if not loaded.
    const std::shared_ptr<ImageAsset>& desk_asset() const { return desk; }

  private:
    static std::string resolve_bg_filename(const std::string& position);
    static std::string resolve_desk_filename(const std::string& position);

    std::string bg_name;
    std::string pos = "wit";
    bool dirty = false;

    std::shared_ptr<ImageAsset> bg;
    std::shared_ptr<ImageAsset> desk;
};

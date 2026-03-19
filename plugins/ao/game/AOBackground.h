#pragma once

#include "ao/asset/AOAssetLibrary.h"
#include "asset/ImageAsset.h"

#include <memory>
#include <string>

/// Manages courtroom background and desk overlay state.
/// Delegates all path resolution to AOAssetLibrary.
class AOBackground {
  public:
    void set(const std::string& background, const std::string& position);
    void set_position(const std::string& position);
    void reload_if_needed(AOAssetLibrary& ao_assets);

    const std::string& background() const { return bg_name; }
    const std::string& position() const { return pos; }

    const std::shared_ptr<ImageAsset>& bg_asset() const { return bg; }
    const std::shared_ptr<ImageAsset>& desk_asset() const { return desk; }

  private:
    std::string bg_name;
    std::string pos = "wit";
    bool dirty = false;

    std::shared_ptr<ImageAsset> bg;
    std::shared_ptr<ImageAsset> desk;
};

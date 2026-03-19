#pragma once

#include "IScenePresenter.h"
#include "asset/ImageAsset.h"
#include "render/Image.h"

#include <memory>
#include <optional>

class CourtroomPresenter : public IScenePresenter {
  public:
    RenderState tick(uint64_t t) override;

  private:
    void load_assets();

    std::shared_ptr<ImageAsset> phoenix_asset;
    std::optional<Image> phoenix_img; // stable ID; pixel data owned by phoenix_asset
};

/**
 * @file CourtroomPresenter.h
 * @brief Stub IScenePresenter that loads and displays a Phoenix Wright image.
 */
#pragma once

#include "IScenePresenter.h"
#include "asset/ImageAsset.h"
#include "render/Image.h"

#include <memory>
#include <optional>

/**
 * @brief Stub scene presenter that loads a phoenix character image.
 *
 * This is a minimal IScenePresenter implementation used for early testing.
 * On the first tick it loads the phoenix character sprite via the asset
 * system, then returns a RenderState containing that image on every
 * subsequent tick.
 */
class CourtroomPresenter : public IScenePresenter {
  public:
    /**
     * @brief Advance the scene by one tick.
     *
     * On the first call, triggers load_assets() to fetch the phoenix sprite.
     * Returns a RenderState containing the loaded image.
     *
     * @param t The current timestamp in milliseconds (monotonic).
     * @return A RenderState with the phoenix character image.
     */
    RenderState tick(uint64_t t) override;

  private:
    /**
     * @brief Load the phoenix character sprite from the asset system.
     *
     * Called once on the first tick. Populates phoenix_asset and phoenix_img.
     */
    void load_assets();

    std::shared_ptr<ImageAsset> phoenix_asset; /**< Decoded phoenix image asset. */
    std::optional<Image> phoenix_img;          /**< Stable render Image; pixel data owned by phoenix_asset. */
};

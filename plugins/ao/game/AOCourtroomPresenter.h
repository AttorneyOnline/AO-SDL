#pragma once

/// @file AOCourtroomPresenter.h
/// @brief AO2-specific courtroom scene presenter — thin orchestration layer.

#include "AOBackground.h"
#include "AOEmotePlayer.h"
#include "AOTextBox.h"
#include "ao/asset/AOAssetLibrary.h"
#include "game/IScenePresenter.h"
#include "asset/ImageAsset.h"

#include <memory>

/// AO2 courtroom scene presenter.
///
/// Owns an AOAssetLibrary that all components share for path resolution.
/// Thin orchestrator: drains events → delegates to components → assembles RenderState.
class AOCourtroomPresenter : public IScenePresenter {
  public:
    RenderState tick(uint64_t t) override;

  private:
    std::unique_ptr<AOAssetLibrary> ao_assets;
    AOBackground background;
    AOEmotePlayer emote_player;
    AOTextBox textbox;

    bool show_desk = true;
    bool current_flip = false;
    bool initialized = false;

    std::shared_ptr<ImageAsset> textbox_overlay;
    bool textbox_dirty = false;

    int evict_timer_ms = 0;

    static constexpr int VIEWPORT_W = 256;
    static constexpr int VIEWPORT_H = 192;
};

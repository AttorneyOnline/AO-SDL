#pragma once

/// @file AOCourtroomPresenter.h
/// @brief AO2-specific courtroom scene presenter — thin orchestration layer.

#include "AOBackground.h"
#include "AOEmotePlayer.h"
#include "game/IScenePresenter.h"

/// AO2 courtroom scene presenter.
///
/// Thin orchestrator: drains events, delegates to AOBackground and
/// AOEmotePlayer, assembles the RenderState. Contains no asset loading
/// logic, no filename conventions, no animation state machine — those
/// live in the components.
///
/// Layers carry shared_ptr<ImageAsset> + frame index — no raw pixel
/// data crosses the triple-buffer. The renderer uploads all frames to
/// the GPU once as a texture array, then selects frames via shader uniform.
class AOCourtroomPresenter : public IScenePresenter {
  public:
    RenderState tick(uint64_t t) override;

  private:
    AOBackground background;
    AOEmotePlayer emote_player;

    bool show_desk = true;
    bool current_flip = false;
};

#pragma once

#include "AOBackground.h"
#include "AOEmotePlayer.h"
#include "AOTextBox.h"
#include "ao/asset/AOAssetLibrary.h"
#include "asset/ImageAsset.h"
#include "ao/game/effects/FlashEffect.h"
#include "ao/game/effects/ScreenshakeEffect.h"
#include "game/IScenePresenter.h"

#include <memory>
#include <vector>

class ISceneEffect;

class AOCourtroomPresenter : public IScenePresenter {
  public:
    AOCourtroomPresenter();
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

    // Scene effects
    ScreenshakeEffect screenshake_;
    FlashEffect flash_{VIEWPORT_W, VIEWPORT_H};
    std::vector<ISceneEffect*> effects_;

    static constexpr int VIEWPORT_W = 256;
    static constexpr int VIEWPORT_H = 192;
};

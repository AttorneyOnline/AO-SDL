#include "ao/game/AOCourtroomPresenter.h"

#include "ao/event/ICMessageEvent.h"
#include "asset/MediaManager.h"
#include "event/BackgroundEvent.h"
#include "event/EventManager.h"
#include "render/Layer.h"
#include "render/RenderState.h"
#include "utils/Log.h"

RenderState AOCourtroomPresenter::tick(uint64_t t) {
    constexpr int delta_ms = 100;

    auto& assets = MediaManager::instance().assets();

    // ---- Drain background events ----
    auto& bg_ch = EventManager::instance().get_channel<BackgroundEvent>();
    while (auto ev = bg_ch.get_event()) {
        background.set(ev->get_background(),
                       ev->get_position().empty() ? background.position() : ev->get_position());
    }

    // ---- Drain IC message events ----
    auto& ic_ch = EventManager::instance().get_channel<ICMessageEvent>();
    while (auto ev = ic_ch.get_event()) {
        if (!ev->get_side().empty()) {
            background.set_position(ev->get_side());
        }

        DeskMod dm = ev->get_desk_mod();
        show_desk = (dm == DeskMod::SHOW || dm == DeskMod::EMOTE_ONLY || dm == DeskMod::EMOTE_ONLY_EX);
        current_flip = ev->get_flip();

        emote_player.start(assets, ev->get_character(), ev->get_emote(),
                           ev->get_pre_emote(), ev->get_emote_mod());
    }

    // ---- Update components ----
    background.reload_if_needed(assets);
    emote_player.tick(delta_ms);

    // Periodically let the asset cache evict unused entries (~every 30 seconds)
    if (t % 300 == 0) {
        assets.evict_unused();
    }

    // ---- Assemble RenderState ----
    // Layers carry shared_ptr<ImageAsset> + frame index.
    // The renderer uploads all frames to the GPU once as a texture array,
    // then selects the frame in the shader. No pixel data crosses the
    // triple-buffer — just asset pointers and frame indices.

    RenderState state;
    LayerGroup scene;

    if (background.bg_asset()) {
        scene.add_layer(0, Layer(background.bg_asset(), 0, 0));
    }

    if (emote_player.has_frame()) {
        int fi = emote_player.current_frame_index();
        auto& asset = emote_player.asset();
        if (t % 10 == 0) { // log every 10 ticks (~1 sec)
            Log::log_print(DEBUG, "Presenter: emote frame=%d/%d state=%d asset=%s",
                           fi, asset ? asset->frame_count() : 0,
                           static_cast<int>(emote_player.state()),
                           asset ? asset->path().c_str() : "null");
        }
        scene.add_layer(5, Layer(asset, fi, 5));
    }

    if (background.desk_asset() && show_desk) {
        scene.add_layer(10, Layer(background.desk_asset(), 0, 10));
    }

    state.add_layer_group(0, scene);
    return state;
}

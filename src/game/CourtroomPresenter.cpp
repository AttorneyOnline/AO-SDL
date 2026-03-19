#include "CourtroomPresenter.h"

#include "asset/MediaManager.h"
#include "render/Layer.h"
#include "render/RenderState.h"

void CourtroomPresenter::load_assets() {
    m_phoenix_asset = MediaManager::instance().assets().image("characters/Phoenix-W/char_icon");

    if (m_phoenix_asset && m_phoenix_asset->frame_count() > 0) {
        const ImageFrame& frame = m_phoenix_asset->frame(0);
        m_phoenix_img.emplace(frame.width, frame.height, frame.pixels.data(), 4);
    }
}

RenderState CourtroomPresenter::tick(uint64_t t) {
    if (!m_phoenix_asset) {
        load_assets();
    }

    RenderState state;

    if (!m_phoenix_img.has_value()) {
        return state;
    }

    uint64_t ticks = t % 20;

    LayerGroup main;

    if (ticks < 10) {
        main.add_layer(0, Layer(*m_phoenix_img, 0));
    }
    else {
        // blank frame — nothing in the layer group
    }

    state.add_layer_group(0, main);
    return state;
}

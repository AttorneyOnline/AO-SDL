#pragma once

#include "asset/ImageAsset.h"
#include "game/IScenePresenter.h"
#include "render/Layer.h"
#include "render/RenderState.h"

#include <memory>
#include <vector>

/**
 * @brief Stub scene presenter with a visible animated GL output.
 *
 * Renders a full-viewport quad (NDC -1…+1) whose colour cycles through three
 * solid frames — red → green → blue — at approximately 1 Hz.  The cycling
 * proves the GL renderer pipeline is alive and producing new frames even while
 * QML screen transitions are in progress.
 *
 * A single GL_TEXTURE_2D_ARRAY is uploaded once (3 × 1×1 pixel frames) and
 * never mutated, so there is no data race between the game thread and the
 * render thread.
 *
 * Replace with the real AO scene presenter (ao::create_presenter()) in Phase 3.
 */
class NullScenePresenter final : public IScenePresenter {
  public:
    NullScenePresenter() {
        // Three 1×1 RGBA frames: vivid red, green, blue.
        // All frames share the same 1×1 dimensions (required by ImageAsset).
        std::vector<DecodedFrame> frames;
        frames.reserve(3);

        for (auto [r, g, b] : {
                 std::tuple<uint8_t, uint8_t, uint8_t>{220,  50,  60},   // red
                 std::tuple<uint8_t, uint8_t, uint8_t>{ 50, 200,  70},   // green
                 std::tuple<uint8_t, uint8_t, uint8_t>{ 50,  80, 220},   // blue
             })
        {
            DecodedFrame f;
            f.pixels      = {r, g, b, 255};
            f.width       = 1;
            f.height      = 1;
            f.duration_ms = 0;
            frames.push_back(std::move(f));
        }

        m_quad = std::make_shared<ImageAsset>(
            "__stub_quad__", "raw", std::move(frames));
    }

    RenderState tick(uint64_t delta_ms) override {
        m_time_ms += delta_ms;

        // Advance frame approximately every 333 ms → full R/G/B cycle ≈ 1 s.
        int frame = static_cast<int>(m_time_ms / 333) % 3;

        // Layer at z-index 0, identity transform → quad fills NDC [-1, 1] × [-1, 1].
        LayerGroup group;
        group.add_layer(0, Layer(m_quad, frame, 0));

        RenderState state;
        state.add_layer_group(0, std::move(group));
        return state;
    }

  private:
    uint64_t m_time_ms = 0;
    std::shared_ptr<ImageAsset> m_quad;
};

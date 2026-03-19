/**
 * @file IScenePresenter.h
 * @brief Interface for game logic producers that generate render state each tick.
 */
#pragma once

#include "render/RenderState.h"

#include <cstdint>

/**
 * @brief Abstract interface for scene presenters.
 *
 * A scene presenter encapsulates the game logic for a particular screen or
 * mode. Each call to tick() advances the simulation and produces a RenderState
 * snapshot that the renderer can display.
 *
 * Implementations are driven by GameThread at approximately 60 Hz.
 */
class IScenePresenter {
  public:
    virtual ~IScenePresenter() = default;

    /**
     * @brief Advance the scene and produce a render snapshot.
     * @param delta_ms Milliseconds elapsed since the last tick.
     * @return A RenderState describing what should be drawn this frame.
     */
    virtual RenderState tick(uint64_t delta_ms) = 0;
};

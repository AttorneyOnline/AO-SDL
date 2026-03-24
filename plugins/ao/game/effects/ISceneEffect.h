#pragma once

class LayerGroup;

/// Base interface for courtroom scene effects (screenshake, flash, zoom, etc.).
///
/// Effects are triggered, tick each frame, and apply modifications to the
/// scene LayerGroup (transforms, layers, visibility, etc.).
class ISceneEffect {
  public:
    virtual ~ISceneEffect() = default;

    /// Start or restart the effect.
    virtual void trigger() = 0;

    /// Stop the effect immediately.
    virtual void stop() = 0;

    /// Advance the effect by delta_ms. Called every frame.
    virtual void tick(int delta_ms) = 0;

    /// Apply the effect's current state to the scene.
    /// Some effects (e.g. SlideEffect) use specialized apply methods for
    /// multi-group scenes and implement this as a no-op.
    virtual void apply(LayerGroup& scene) = 0;

    /// Whether the effect is currently active (playing or needs to apply).
    virtual bool is_active() const = 0;
};

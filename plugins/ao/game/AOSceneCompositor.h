#pragma once

class AOBackground;
struct ActiveICState;
class AOTextBox;
class RenderState;

/// Composes the courtroom scene into a RenderState each frame.
///
/// Reads from the presenter's components (background, IC states, textbox)
/// and produces a layered render output. Separated from the presenter to
/// keep tick() focused on state advancement.
class AOSceneCompositor {
  public:
    RenderState compose(const AOBackground& background, const ActiveICState* active_ic,
                        const ActiveICState* departing_ic, AOTextBox& textbox, float scene_time_s);
};

#pragma once

#include "render/Math.h"

#include <cstdint>

class Transform {
  public:
    Transform();

    Mat4 get_local_transform() const;

    void rotate(float degrees);
    void scale(Vec2 scale);
    void translate(Vec2 offset);
    void zindex(uint16_t index);

    static float get_aspect_ratio();
    static void set_aspect_ratio(float aspect);

  protected:
    Mat4 transform;

    float rotation;
    Vec3 scaling;
    Vec3 translation;

    static float aspect_ratio;

  private:
    void recalculate();
};

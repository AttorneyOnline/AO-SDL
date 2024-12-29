#ifndef RENDER_TRANSFORM_H
#define RENDER_TRANSFORM_H

#include <glm/glm.hpp>

class Transform {
  public:
    Transform();

    glm::mat4 get_local_transform();

    void rotate(float degrees);
    void scale(glm::vec2 scale);
    void translate(glm::vec2 offset);
    void zindex(uint16_t index);

    static float get_aspect_ratio();
    static void set_aspect_ratio(float aspect);

  protected:
    glm::mat4 transform;

    float rotation;
    glm::vec3 scaling;
    glm::vec3 translation;

    static float aspect_ratio;

  private:
    void recalculate();
};

#endif

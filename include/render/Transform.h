/**
 * @file Transform.h
 * @brief 2D transformation matrix (rotate, scale, translate, z-index).
 */
#pragma once

#include <glm/glm.hpp>

/**
 * @brief A 2D transformation represented as a 4x4 matrix (for use with GLM).
 *
 * Encapsulates rotation, scaling, and translation, and exposes a z-index for
 * draw ordering. The matrix is lazily recalculated when any component changes.
 *
 * A global aspect ratio is shared across all Transform instances via a static
 * member.
 */
class Transform {
  public:
    /** @brief Construct an identity transform. */
    Transform();

    /**
     * @brief Get the composed local transformation matrix.
     * @return A 4x4 matrix combining rotation, scale, and translation.
     */
    glm::mat4 get_local_transform();

    /**
     * @brief Set the rotation angle.
     * @param degrees Rotation in degrees.
     */
    void rotate(float degrees);

    /**
     * @brief Set the scale factors.
     * @param scale A 2D vector containing the X and Y scale factors.
     */
    void scale(glm::vec2 scale);

    /**
     * @brief Set the translation offset.
     * @param offset A 2D vector containing the X and Y translation.
     */
    void translate(glm::vec2 offset);

    /**
     * @brief Set the z-index for draw ordering.
     * @param index Z-index value; lower values are drawn behind higher values.
     */
    void zindex(uint16_t index);

    /**
     * @brief Get the global aspect ratio.
     * @return The current aspect ratio shared by all Transform instances.
     */
    static float get_aspect_ratio();

    /**
     * @brief Set the global aspect ratio.
     * @param aspect The new aspect ratio (width / height).
     */
    static void set_aspect_ratio(float aspect);

  protected:
    glm::mat4 transform; ///< The composed transformation matrix.

    float rotation;        ///< Rotation angle in degrees.
    glm::vec3 scaling;     ///< Scale factors (x, y, z).
    glm::vec3 translation; ///< Translation offset (x, y, z).

    static float aspect_ratio; ///< Global aspect ratio shared across all instances.

  private:
    /**
     * @brief Recompute the transformation matrix from the current components.
     */
    void recalculate();
};

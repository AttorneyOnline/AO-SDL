#include "Transform.h"

#include <glm/gtc/matrix_transform.hpp>

float Transform::aspect_ratio = 1.0f;

Transform::Transform() : transform(1.0f), rotation(0.0f), scaling(1.0f), translation(0.0f) {
}

glm::mat4 Transform::get_local_transform() {
    return transform;
}

void Transform::rotate(float degrees) {
    rotation = degrees;
    recalculate();
}

void Transform::scale(glm::vec2 scale) {
    scaling = glm::vec3(scale.x, scale.y, 1.0f);
    recalculate();
}

void Transform::translate(glm::vec2 offset) {
    translation = glm::vec3(offset.x, offset.y, 0.0f);
    recalculate();
}

void Transform::zindex(uint16_t index) {
    // Nudge one bump forward so that idx 0 meets the clipping plane.
    // This means index=UINT16_MAX gets clipped, but that is okay

    translation.z = (((float)index / UINT16_MAX) * -2.0f) + (1.0f - (1.0f / UINT16_MAX));
    // Log::log_print(LogLevel::DEBUG, "Set z-index to %d (%f in NDC)", index, translation.z);
    recalculate();
}

float Transform::get_aspect_ratio() {
    return aspect_ratio;
}

void Transform::set_aspect_ratio(float aspect) {
    aspect_ratio = aspect;
}

void Transform::recalculate() {
    glm::mat4 basis(1.0f);

    // Translate
    basis = glm::translate(basis, translation);

    // Rotate with aspect ratio correction
    basis = glm::scale(basis, {1.0f / aspect_ratio, 1.0f, 1.0f});
    basis = glm::rotate(basis, glm::radians(rotation), glm::vec3(0, 0, 1));
    basis = glm::scale(basis, {aspect_ratio, 1.0f, 1.0f});

    // Scale
    basis = glm::scale(basis, scaling);

    transform = basis;
}

#include "render/Transform.h"

float Transform::aspect_ratio = 1.0f;

Transform::Transform() : transform(Mat4::identity()), rotation(0.0f), scaling(1.0f), translation(0.0f) {
}

Mat4 Transform::get_local_transform() const {
    return transform;
}

void Transform::rotate(float degrees) {
    rotation = degrees;
    recalculate();
}

void Transform::scale(Vec2 s) {
    scaling = Vec3(s.x, s.y, 1.0f);
    recalculate();
}

void Transform::translate(Vec2 offset) {
    translation = Vec3(offset.x, offset.y, 0.0f);
    recalculate();
}

void Transform::zindex(uint16_t index) {
    translation.z = (((float)index / UINT16_MAX) * -2.0f) + (1.0f - (1.0f / UINT16_MAX));
    recalculate();
}

float Transform::get_aspect_ratio() {
    return aspect_ratio;
}

void Transform::set_aspect_ratio(float aspect) {
    aspect_ratio = aspect;
}

void Transform::recalculate() {
    Mat4 basis = Mat4::identity();

    basis = ::translate(basis, translation);

    basis = ::scale(basis, Vec3(1.0f / aspect_ratio, 1.0f, 1.0f));
    basis = ::rotate(basis, radians(rotation), Vec3(0, 0, 1));
    basis = ::scale(basis, Vec3(aspect_ratio, 1.0f, 1.0f));

    basis = ::scale(basis, scaling);

    transform = basis;
}

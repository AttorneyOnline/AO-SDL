#include "Image.h"

#include <random>

#include <GL/glew.h>

Image::Image(int width, int height, uint8_t* pixels, int num_channels)
    : width(width), height(height), num_channels(num_channels), pixels(pixels) {
    // TODO: Replace with hash of image data
    std::random_device rd;
    std::mt19937_64 random(rd());
    id = random();
}

const uint8_t* Image::get_pixels() {
    return pixels;
}

int Image::get_width() {
    return width;
}

int Image::get_height() {
    return height;
}

int Image::get_num_channels() {
    return num_channels;
}

uint64_t Image::get_id() {
    return id;
}

void Image::prepare() {
    // stuff in gpu thread
}

Animation::Animation(int width, int height, uint8_t* pixels, int num_channels)
    : Image(width, height, pixels, num_channels) {
}

void Animation::update_image(uint8_t* pixels) {
    this->pixels = pixels;
}

void Animation::prepare() {
    // stuff in gpu thread
}

#pragma once

#include <cstdint>

class Image {
  public:
    Image(int width, int height, uint8_t* pixels, int num_channels);
    uint64_t get_id();

    const uint8_t* get_pixels();
    int get_width();
    int get_height();
    int get_num_channels();

    virtual void prepare();

  protected:
    int width;
    int height;
    int num_channels;
    uint8_t* pixels;

    uint64_t id;
};

class Animation : public Image {
  public:
    Animation(int width, int height, uint8_t* pixels, int num_channels);

    virtual void prepare();

    void update_image(uint8_t* pixels);
};

/**
 * @file Image.h
 * @brief CPU-side image and animation data (pixels, dimensions, unique ID).
 */
#pragma once

#include <cstdint>

/**
 * @brief CPU-side image holding raw pixel data, dimensions, and a unique identifier.
 *
 * Image does not own the pixel buffer; the caller must ensure the pointer
 * remains valid for the lifetime of the Image.
 */
class Image {
  public:
    /**
     * @brief Construct an Image.
     * @param width        Width in pixels.
     * @param height       Height in pixels.
     * @param pixels       Pointer to the raw pixel data (not owned).
     * @param num_channels Number of color channels (e.g. 3 for RGB, 4 for RGBA).
     */
    Image(int width, int height, const uint8_t* pixels, int num_channels);

    /**
     * @brief Get the unique identifier for this image.
     * @return A 64-bit ID that uniquely identifies this image instance.
     */
    uint64_t get_id();

    /**
     * @brief Get the raw pixel data.
     * @return Pointer to the pixel buffer.
     */
    const uint8_t* get_pixels();

    /**
     * @brief Get the image width.
     * @return Width in pixels.
     */
    int get_width();

    /**
     * @brief Get the image height.
     * @return Height in pixels.
     */
    int get_height();

    /**
     * @brief Get the number of color channels.
     * @return Channel count (e.g. 3 for RGB, 4 for RGBA).
     */
    int get_num_channels();

    /**
     * @brief Prepare the image for use (e.g. upload to GPU).
     *
     * The default implementation is a no-op. Subclasses may override to
     * perform backend-specific preparation.
     */
    virtual void prepare();

  protected:
    int width;                ///< Width in pixels.
    int height;               ///< Height in pixels.
    int num_channels;         ///< Number of color channels.
    const uint8_t* pixels;    ///< Raw pixel data (not owned).

    uint64_t id;              ///< Unique image identifier.
};

/**
 * @brief An animated image whose pixel data can be updated each frame.
 *
 * Extends Image to allow in-place pixel buffer replacement for animations.
 */
class Animation : public Image {
  public:
    /**
     * @brief Construct an Animation.
     * @param width        Width in pixels.
     * @param height       Height in pixels.
     * @param pixels       Pointer to the initial pixel data (not owned).
     * @param num_channels Number of color channels.
     */
    Animation(int width, int height, uint8_t* pixels, int num_channels);

    /**
     * @brief Prepare the animation for the current frame.
     *
     * Overrides Image::prepare() to handle animation-specific setup.
     */
    virtual void prepare();

    /**
     * @brief Replace the current pixel data with a new frame.
     * @param pixels Pointer to the new pixel data (not owned).
     */
    void update_image(const uint8_t* pixels);
};

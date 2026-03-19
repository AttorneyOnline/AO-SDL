/**
 * @file Texture.h
 * @brief Backend-agnostic 2D texture handle using the pimpl idiom.
 */
#pragma once

#include <cstdint>
#include <memory>

/**
 * @brief Backend-agnostic 2D texture handle.
 *
 * The internal GPU representation is hidden behind a pimpl (Impl struct)
 * so that callers never need to include backend-specific headers (e.g. GL).
 * The Impl is provided by the active render backend plugin.
 *
 * This class is **move-only** because it manages a GPU resource that should
 * not be duplicated.
 */
class Texture2D {
  public:
    /**
     * @brief Create a texture from raw pixel data.
     *
     * Uploads the pixel data to the GPU via the backend-provided Impl.
     *
     * @param width    Texture width in pixels.
     * @param height   Texture height in pixels.
     * @param pixels   Pointer to the raw pixel data.
     * @param channels Number of color channels (e.g. 3 for RGB, 4 for RGBA).
     */
    Texture2D(int width, int height, const uint8_t* pixels, int channels);

    /**
     * @brief Destructor. Releases the underlying GPU resource.
     */
    ~Texture2D();

    /** @brief Move constructor. Transfers GPU resource ownership. */
    Texture2D(Texture2D&&) noexcept;

    /** @brief Move assignment operator. Transfers GPU resource ownership. */
    Texture2D& operator=(Texture2D&&) noexcept;

    /** @brief Copy constructor is deleted (GPU resource is non-copyable). */
    Texture2D(const Texture2D&) = delete;

    /** @brief Copy assignment is deleted (GPU resource is non-copyable). */
    Texture2D& operator=(const Texture2D&) = delete;

    /**
     * @brief Get an opaque GPU handle for this texture.
     *
     * The returned value can be passed to toolkit functions that expect a
     * texture ID (e.g. ImGui::ImageButton).
     *
     * @return Opaque 64-bit GPU texture handle.
     */
    uint64_t get_id() const;

    /**
     * @brief Get the texture width.
     * @return Width in pixels.
     */
    int get_width() const;

    /**
     * @brief Get the texture height.
     * @return Height in pixels.
     */
    int get_height() const;

  private:
    /** @brief Forward-declared implementation holding backend-specific GPU state. */
    struct Impl;
    std::unique_ptr<Impl> impl; ///< Pointer to the backend-specific implementation.
};

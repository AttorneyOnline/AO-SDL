#pragma once

#include <cstdint>
#include <memory>

// Backend-agnostic 2D texture handle.
// Internal GPU representation is hidden behind pimpl — callers never see GL headers.
class Texture2D {
  public:
    Texture2D(int width, int height, const uint8_t* pixels, int channels);
    ~Texture2D();

    Texture2D(Texture2D&&) noexcept;
    Texture2D& operator=(Texture2D&&) noexcept;

    Texture2D(const Texture2D&) = delete;
    Texture2D& operator=(const Texture2D&) = delete;

    // Opaque GPU handle (e.g. for ImGui ImageButton)
    uint64_t get_id() const;
    int get_width() const;
    int get_height() const;

  private:
    struct Impl;
    std::unique_ptr<Impl> m_impl;
};

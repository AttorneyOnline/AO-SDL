#pragma once

#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <cstring>

#ifdef _WIN32
#include <malloc.h>
#else
#include <unistd.h>
#endif

/**
 * @brief Page-aligned memory buffer for zero-copy GPU texture sharing.
 *
 * On Apple Silicon, Metal can create textures backed by existing memory
 * via newBufferWithBytesNoCopy — but the memory must be page-aligned.
 * This buffer guarantees that alignment.
 */
class AlignedBuffer {
  public:
    AlignedBuffer() = default;

    explicit AlignedBuffer(size_t size, uint8_t fill = 0) : size_(size) {
        if (size_ == 0)
            return;
        data_ = static_cast<uint8_t*>(aligned_alloc_impl(page_size(), round_up(size_, page_size())));
        std::memset(data_, fill, size_);
    }

    ~AlignedBuffer() {
        free_impl();
    }

    // Copy
    AlignedBuffer(const AlignedBuffer& other) : size_(other.size_) {
        if (size_ == 0)
            return;
        data_ = static_cast<uint8_t*>(aligned_alloc_impl(page_size(), round_up(size_, page_size())));
        std::memcpy(data_, other.data_, size_);
    }

    AlignedBuffer& operator=(const AlignedBuffer& other) {
        if (this == &other)
            return *this;
        free_impl();
        size_ = other.size_;
        if (size_ > 0) {
            data_ = static_cast<uint8_t*>(aligned_alloc_impl(page_size(), round_up(size_, page_size())));
            std::memcpy(data_, other.data_, size_);
        }
        return *this;
    }

    // Move
    AlignedBuffer(AlignedBuffer&& other) noexcept : data_(other.data_), size_(other.size_) {
        other.data_ = nullptr;
        other.size_ = 0;
    }

    AlignedBuffer& operator=(AlignedBuffer&& other) noexcept {
        if (this == &other)
            return *this;
        free_impl();
        data_ = other.data_;
        size_ = other.size_;
        other.data_ = nullptr;
        other.size_ = 0;
        return *this;
    }

    uint8_t* data() {
        return data_;
    }
    const uint8_t* data() const {
        return data_;
    }
    size_t size() const {
        return size_;
    }
    bool empty() const {
        return size_ == 0;
    }

    uint8_t& operator[](size_t i) {
        return data_[i];
    }
    const uint8_t& operator[](size_t i) const {
        return data_[i];
    }

    uint8_t* begin() {
        return data_;
    }
    uint8_t* end() {
        return data_ + size_;
    }
    const uint8_t* begin() const {
        return data_;
    }
    const uint8_t* end() const {
        return data_ + size_;
    }

    /// Copy from raw pointer range into a new buffer.
    static AlignedBuffer from_range(const uint8_t* begin, const uint8_t* end) {
        AlignedBuffer buf;
        buf.size_ = static_cast<size_t>(end - begin);
        if (buf.size_ > 0) {
            buf.data_ = static_cast<uint8_t*>(aligned_alloc_impl(page_size(), round_up(buf.size_, page_size())));
            std::memcpy(buf.data_, begin, buf.size_);
        }
        return buf;
    }

    /// Allocated size rounded up to page boundary (what the OS actually gave us).
    size_t allocated_size() const {
        return round_up(size_, page_size());
    }

    static size_t page_size() {
#ifdef _WIN32
        return 4096;
#else
        static const size_t ps = static_cast<size_t>(sysconf(_SC_PAGESIZE));
        return ps;
#endif
    }

  private:
    uint8_t* data_ = nullptr;
    size_t size_ = 0;

    static size_t round_up(size_t value, size_t alignment) {
        return (value + alignment - 1) & ~(alignment - 1);
    }

    static void* aligned_alloc_impl(size_t alignment, size_t size) {
        if (size == 0)
            return nullptr;
#ifdef _WIN32
        return _aligned_malloc(size, alignment);
#else
        void* ptr = nullptr;
        posix_memalign(&ptr, alignment, size);
        return ptr;
#endif
    }

    void free_impl() {
        if (!data_)
            return;
#ifdef _WIN32
        _aligned_free(data_);
#else
        std::free(data_);
#endif
        data_ = nullptr;
        size_ = 0;
    }
};

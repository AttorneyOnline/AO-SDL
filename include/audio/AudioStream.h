#pragma once

#include <atomic>
#include <condition_variable>
#include <cstdint>
#include <memory>
#include <mutex>
#include <thread>
#include <vector>

/**
 * @brief Lock-free single-producer single-consumer ring buffer for float PCM data.
 *
 * Producer: decode thread writes decoded PCM.
 * Consumer: SDL audio callback reads PCM for mixing.
 */
class PcmRingBuffer {
  public:
    explicit PcmRingBuffer(size_t capacity);

    /// Write samples. Returns number actually written (may be less if full).
    size_t write(const float* data, size_t count);

    /// Read samples. Returns number actually read (may be less if empty).
    size_t read(float* data, size_t count);

    /// Number of samples available to read.
    size_t available() const;

    /// Reset both pointers to empty.
    void reset();

  private:
    std::vector<float> buf_;
    size_t mask_;
    std::atomic<size_t> write_head_{0};
    std::atomic<size_t> read_head_{0};
};

/**
 * @brief Streaming audio decoder.
 *
 * Fed raw compressed bytes (from an HTTP download thread) and decodes them
 * on a background thread into a PCM ring buffer that the SDL audio callback
 * reads from. This allows audio playback to start before the full file is
 * downloaded.
 *
 * Thread model:
 *   HTTP download thread → feed() / mark_complete()
 *   Internal decode thread → reads raw bytes, decodes, writes PCM to ring buffer
 *   SDL audio callback → read_frames()
 */
class AudioStream : public std::enable_shared_from_this<AudioStream> {
  public:
    AudioStream();
    ~AudioStream();

    // --- Called by download thread ---

    /// Append raw compressed audio data.
    void feed(const uint8_t* data, size_t len);

    /// Signal that all raw data has been fed (EOF).
    void mark_complete();

    // --- Called by SDL audio callback (non-blocking) ---

    /// Read decoded interleaved stereo float32 frames.
    /// Returns number of frames actually read. If less than count, caller
    /// should pad with silence (buffer underrun or stream finished).
    int read_frames(float* output, int frame_count);

    // --- Called by AudioThread / control ---

    /// Stop decoding and release resources. Safe to call from any thread.
    void cancel();

    /// Set loop start/end in PCM samples (48kHz).
    /// If loop_end is 0, loops to end of stream.
    /// AO2 convention: these come from a .txt file alongside the music file.
    void set_loop_points(int64_t loop_start_samples, int64_t loop_end_samples = 0);

    /// Enable/disable looping. When enabled, the decode thread automatically
    /// seeks back to loop_start when it reaches loop_end (or EOF).
    void set_looping(bool loop);

    /// True once the decoder has parsed headers and started producing PCM.
    bool is_ready() const {
        return ready_.load(std::memory_order_acquire);
    }

    /// True when all data has been decoded and the ring buffer is drained.
    bool is_finished() const {
        return finished_.load(std::memory_order_acquire);
    }

    bool is_cancelled() const {
        return stop_source_.stop_requested();
    }

    uint32_t sample_rate() const {
        return 48000;
    }
    uint32_t channels() const {
        return 2;
    }

    /// Number of decoded PCM samples available in the ring buffer.
    size_t buffered_samples() const {
        return pcm_ring_.available();
    }

    /// Whether the stream is set to loop.
    bool is_looping() const {
        return looping_.load(std::memory_order_acquire);
    }

    /// Loop start point in PCM samples.
    int64_t loop_start() const {
        return loop_start_.load(std::memory_order_acquire);
    }

    /// Loop end point in PCM samples (0 = end of stream).
    int64_t loop_end() const {
        return loop_end_.load(std::memory_order_acquire);
    }

    // --- opusfile callback helpers (public for C callback access) ---

    /// Blocking read from raw buffer. Returns 0 on EOF or cancel.
    int stream_read(uint8_t* buf, int count);

    /// Seek in raw buffer.
    int stream_seek(int64_t offset, int whence);

    /// Current position in raw buffer.
    int64_t stream_tell();

  private:
    void decode_thread_func(std::stop_token st);

    // --- StreamBuffer: raw compressed data ---
    std::vector<uint8_t> raw_data_;
    size_t raw_read_pos_ = 0;
    bool raw_complete_ = false;
    mutable std::mutex raw_mutex_;
    std::condition_variable raw_cv_;

    // --- PCM output ring buffer ---
    PcmRingBuffer pcm_ring_;

    // --- Loop points (in PCM samples at 48kHz) ---
    std::atomic<int64_t> loop_start_{0};
    std::atomic<int64_t> loop_end_{0}; // 0 = no loop end (loop to end of stream)
    std::atomic<bool> looping_{false};

    // --- State ---
    std::atomic<bool> ready_{false};
    std::atomic<bool> finished_{false};

    std::stop_source stop_source_;
    std::jthread decode_thread_;
};

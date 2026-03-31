#include "audio/AudioStream.h"
#include "utils/Log.h"

#include <opusfile.h>

#define MA_NO_DEVICE_IO
#define MA_NO_GENERATION
#define MA_NO_ENGINE
#define MA_NO_NODE_GRAPH
#define MA_NO_RESOURCE_MANAGER
#include "miniaudio.h"

#include <algorithm>
#include <cstring>

// =============================================================================
// PcmRingBuffer
// =============================================================================

static size_t next_power_of_2(size_t v) {
    v--;
    v |= v >> 1;
    v |= v >> 2;
    v |= v >> 4;
    v |= v >> 8;
    v |= v >> 16;
    v |= v >> 32;
    return v + 1;
}

PcmRingBuffer::PcmRingBuffer(size_t capacity) : buf_(next_power_of_2(capacity)), mask_(buf_.size() - 1) {
}

size_t PcmRingBuffer::write(const float* data, size_t count) {
    size_t w = write_head_.load(std::memory_order_relaxed);
    size_t r = read_head_.load(std::memory_order_acquire);
    size_t free = buf_.size() - (w - r);
    size_t to_write = std::min(count, free);

    for (size_t i = 0; i < to_write; ++i)
        buf_[(w + i) & mask_] = data[i];

    write_head_.store(w + to_write, std::memory_order_release);
    return to_write;
}

size_t PcmRingBuffer::read(float* data, size_t count) {
    size_t r = read_head_.load(std::memory_order_relaxed);
    size_t w = write_head_.load(std::memory_order_acquire);
    size_t avail = w - r;
    size_t to_read = std::min(count, avail);

    for (size_t i = 0; i < to_read; ++i)
        data[i] = buf_[(r + i) & mask_];

    read_head_.store(r + to_read, std::memory_order_release);
    return to_read;
}

size_t PcmRingBuffer::available() const {
    size_t r = read_head_.load(std::memory_order_acquire);
    size_t w = write_head_.load(std::memory_order_acquire);
    return w - r;
}

void PcmRingBuffer::reset() {
    read_head_.store(0, std::memory_order_release);
    write_head_.store(0, std::memory_order_release);
}

// =============================================================================
// AudioStream — opusfile callbacks
// =============================================================================

// These are C callbacks for opusfile. The void* _stream points to AudioStream.

static int opus_read_cb(void* _stream, unsigned char* _ptr, int _nbytes) {
    auto* s = static_cast<AudioStream*>(_stream);
    return s->is_cancelled() ? 0 : reinterpret_cast<AudioStream*>(_stream)->stream_read(_ptr, _nbytes);
}

static int opus_seek_cb(void* _stream, opus_int64 _offset, int _whence) {
    return static_cast<AudioStream*>(_stream)->stream_seek(_offset, _whence);
}

static opus_int64 opus_tell_cb(void* _stream) {
    return static_cast<AudioStream*>(_stream)->stream_tell();
}

static const OpusFileCallbacks opus_callbacks = {opus_read_cb, opus_seek_cb, opus_tell_cb, nullptr};

// =============================================================================
// AudioStream — StreamBuffer read/seek/tell
// =============================================================================

int AudioStream::stream_read(uint8_t* buf, int count) {
    std::unique_lock lock(raw_mutex_);

    // Wait until data is available, EOF, or cancelled
    raw_cv_.wait(lock,
                 [&] { return raw_read_pos_ < raw_data_.size() || raw_complete_ || stop_source_.stop_requested(); });

    if (stop_source_.stop_requested())
        return 0;

    size_t avail = raw_data_.size() - raw_read_pos_;
    if (avail == 0)
        return 0; // EOF

    size_t to_read = std::min(static_cast<size_t>(count), avail);
    std::memcpy(buf, raw_data_.data() + raw_read_pos_, to_read);
    raw_read_pos_ += to_read;
    return static_cast<int>(to_read);
}

int AudioStream::stream_seek(int64_t offset, int whence) {
    std::lock_guard lock(raw_mutex_);

    int64_t new_pos;
    switch (whence) {
    case SEEK_SET:
        new_pos = offset;
        break;
    case SEEK_CUR:
        new_pos = static_cast<int64_t>(raw_read_pos_) + offset;
        break;
    case SEEK_END:
        if (!raw_complete_)
            return -1; // Can't seek from end if we don't know the size yet
        new_pos = static_cast<int64_t>(raw_data_.size()) + offset;
        break;
    default:
        return -1;
    }

    if (new_pos < 0 || (!raw_complete_ && static_cast<size_t>(new_pos) > raw_data_.size()))
        return -1;

    raw_read_pos_ = static_cast<size_t>(new_pos);
    return 0;
}

int64_t AudioStream::stream_tell() {
    std::lock_guard lock(raw_mutex_);
    return static_cast<int64_t>(raw_read_pos_);
}

// =============================================================================
// AudioStream — public API
// =============================================================================

// ~2 seconds of stereo 48kHz PCM
static constexpr size_t RING_CAPACITY = 48000 * 2 * 2;

AudioStream::AudioStream() : pcm_ring_(RING_CAPACITY) {
    // Decode thread is started lazily after mark_complete() to ensure
    // all data is available before decoding begins (avoids stuttering
    // from network jitter during streaming).
}

AudioStream::~AudioStream() {
    cancel();
    // jthread destructor auto-joins
}

void AudioStream::feed(const uint8_t* data, size_t len) {
    {
        std::lock_guard lock(raw_mutex_);
        raw_data_.insert(raw_data_.end(), data, data + len);
    }
    raw_cv_.notify_one();
}

void AudioStream::mark_complete() {
    {
        std::lock_guard lock(raw_mutex_);
        raw_complete_ = true;
    }
    raw_cv_.notify_one();

    // Start the decode thread now that all data is available.
    if (!decode_thread_.joinable() && !stop_source_.stop_requested() && !raw_data_.empty()) {
        decode_thread_ = std::jthread([this](std::stop_token) { decode_thread_func(stop_source_.get_token()); });
    }
    else if (!decode_thread_.joinable()) {
        finished_.store(true, std::memory_order_release);
    }
}

int AudioStream::read_frames(float* output, int frame_count) {
    size_t samples_wanted = static_cast<size_t>(frame_count) * 2;
    size_t got = pcm_ring_.read(output, samples_wanted);
    return static_cast<int>(got / 2);
}

void AudioStream::cancel() {
    stop_source_.request_stop();
    raw_cv_.notify_all();
}

void AudioStream::set_loop_points(int64_t loop_start_samples, int64_t loop_end_samples) {
    loop_start_.store(loop_start_samples, std::memory_order_release);
    loop_end_.store(loop_end_samples, std::memory_order_release);
}

void AudioStream::set_looping(bool loop) {
    looping_.store(loop, std::memory_order_release);
}

// =============================================================================
// AudioStream — decode thread
// =============================================================================

void AudioStream::decode_thread_func(std::stop_token /*st*/) {
    float buf[5760 * 2]; // Max opus frame size * stereo

    bool first_pass = true;

    for (;;) {
        // Open (or reopen) the decoder from the current raw_read_pos_
        int error = 0;
        OggOpusFile* of = op_open_callbacks(this, &opus_callbacks, nullptr, 0, &error);
        if (!of) {
            Log::log_print(DEBUG, "AudioStream: opus failed (error %d), trying miniaudio fallback", error);

            // Miniaudio fallback for MP3/OGG/WAV/FLAC
            ma_decoder_config ma_cfg = ma_decoder_config_init(ma_format_f32, 2, 48000);
            ma_decoder ma_dec;
            std::vector<uint8_t> raw_copy;
            {
                std::lock_guard lock(raw_mutex_);
                raw_copy = raw_data_;
            }

            if (ma_decoder_init_memory(raw_copy.data(), raw_copy.size(), &ma_cfg, &ma_dec) != MA_SUCCESS) {
                Log::log_print(WARNING, "AudioStream: miniaudio also failed (%zu bytes)", raw_copy.size());
                break;
            }

            ready_.store(true, std::memory_order_release);
            Log::log_print(DEBUG, "AudioStream: miniaudio decoder opened");

            int64_t ma_loop_start = loop_start_.load(std::memory_order_acquire);
            int64_t ma_loop_end = loop_end_.load(std::memory_order_acquire);
            bool ma_first_pass = true;

            for (;;) {
                if (stop_source_.stop_requested())
                    break;

                // On loop passes, seek to loop_start
                if (!ma_first_pass && looping_.load(std::memory_order_acquire)) {
                    ma_uint64 seek_frame = ma_loop_start > 0 ? (ma_uint64)ma_loop_start : 0;
                    ma_decoder_seek_to_pcm_frame(&ma_dec, seek_frame);
                    Log::log_print(INFO, "AudioStream: miniaudio looping to frame %llu",
                                   (unsigned long long)seek_frame);
                }
                ma_first_pass = false;

                // Decode loop
                constexpr size_t CHUNK_FRAMES = 4096;
                float ma_buf[CHUNK_FRAMES * 2];
                bool reached_end = false;
                while (!stop_source_.stop_requested()) {
                    // Check loop end point
                    if (ma_loop_end > 0) {
                        ma_uint64 cursor = 0;
                        ma_decoder_get_cursor_in_pcm_frames(&ma_dec, &cursor);
                        if ((int64_t)cursor >= ma_loop_end) {
                            reached_end = true;
                            break;
                        }
                    }

                    ma_uint64 frames_read = 0;
                    ma_decoder_read_pcm_frames(&ma_dec, ma_buf, CHUNK_FRAMES, &frames_read);
                    if (frames_read == 0) {
                        reached_end = true;
                        break;
                    }

                    size_t samples = static_cast<size_t>(frames_read) * 2;
                    size_t written = 0;
                    while (written < samples && !stop_source_.stop_requested()) {
                        size_t n = pcm_ring_.write(ma_buf + written, samples - written);
                        written += n;
                        if (written < samples)
                            std::this_thread::sleep_for(std::chrono::milliseconds(1));
                    }
                }

                if (!reached_end || stop_source_.stop_requested())
                    break;
                if (!looping_.load(std::memory_order_acquire))
                    break;
            }

            ma_decoder_uninit(&ma_dec);
            break;
        }

        if (!ready_.load(std::memory_order_acquire)) {
            ready_.store(true, std::memory_order_release);
            Log::log_print(DEBUG, "AudioStream: decoder opened successfully");
        }

        int64_t loop_start = loop_start_.load(std::memory_order_acquire);
        int64_t loop_end = loop_end_.load(std::memory_order_acquire);

        // First pass plays from the beginning of the file.
        // Subsequent loops seek to loop_start.
        if (!first_pass && loop_start > 0) {
            int seek_err = op_pcm_seek(of, loop_start);
            if (seek_err < 0)
                Log::log_print(WARNING, "AudioStream: seek to loop_start %lld failed (%d)", (long long)loop_start,
                               seek_err);
            else
                Log::log_print(INFO, "AudioStream: seeked to loop_start %lld", (long long)loop_start);
        }
        first_pass = false;

        // Decode loop
        while (!stop_source_.stop_requested()) {
            // Check loop end point
            if (loop_end > 0) {
                ogg_int64_t current = op_pcm_tell(of);
                if (current >= loop_end)
                    break;
            }

            int ret = op_read_float_stereo(of, buf, sizeof(buf) / sizeof(buf[0]));
            if (ret < 0) {
                Log::log_print(DEBUG, "AudioStream: decode error %d", ret);
                continue;
            }
            if (ret == 0)
                break; // EOF

            size_t samples = static_cast<size_t>(ret) * 2;
            size_t written = 0;
            while (written < samples && !stop_source_.stop_requested()) {
                size_t n = pcm_ring_.write(buf + written, samples - written);
                written += n;
                if (written < samples)
                    std::this_thread::sleep_for(std::chrono::milliseconds(1));
            }
        }

        op_free(of);

        if (stop_source_.stop_requested())
            break;

        bool should_loop = looping_.load(std::memory_order_acquire);
        Log::log_print(INFO, "AudioStream: decode pass finished, looping=%d, cancelled=%d", should_loop,
                       stop_source_.stop_requested());

        // If looping, reset raw read position and reopen the decoder
        if (should_loop) {
            Log::log_print(INFO, "AudioStream: looping back to start (raw_size=%zu)", raw_data_.size());
            {
                std::lock_guard lock(raw_mutex_);
                raw_read_pos_ = 0;
            }
            continue; // Reopen decoder from the top
        }

        break; // Not looping → done
    }

    finished_.store(true, std::memory_order_release);
}

#include "MiniaudioDevice.h"

#include "utils/Log.h"

#include <algorithm>
#include <cstring>

MiniaudioDevice::~MiniaudioDevice() {
    close();
}

bool MiniaudioDevice::open() {
    // Configure the miniaudio context with iOS audio session settings
    ma_context_config ctx_config = ma_context_config_init();
#ifdef __APPLE__
    ctx_config.coreaudio.sessionCategory = ma_ios_session_category_playback;
    ctx_config.coreaudio.sessionCategoryOptions = ma_ios_session_category_option_mix_with_others;
#endif

    Log::log_print(INFO, "MiniaudioDevice: initializing context...");
    ma_result result = ma_context_init(nullptr, 0, &ctx_config, &context_);
    if (result != MA_SUCCESS) {
        Log::log_print(ERR, "MiniaudioDevice: failed to init context: %d", result);
        return false;
    }
    context_open_ = true;
    Log::log_print(INFO, "MiniaudioDevice: context initialized, backend=%d", context_.backend);

    ma_device_config config = ma_device_config_init(ma_device_type_playback);
    config.playback.format = ma_format_f32;
    config.playback.channels = 2;
    config.sampleRate = SAMPLE_RATE;
    config.dataCallback = data_callback;
    config.pUserData = this;
    config.periodSizeInFrames = 4096;

    Log::log_print(INFO, "MiniaudioDevice: initializing device...");
    result = ma_device_init(&context_, &config, &device_);
    if (result != MA_SUCCESS) {
        Log::log_print(ERR, "MiniaudioDevice: failed to init device: %d", result);
        ma_context_uninit(&context_);
        context_open_ = false;
        return false;
    }

    Log::log_print(INFO, "MiniaudioDevice: starting device...");
    result = ma_device_start(&device_);
    if (result != MA_SUCCESS) {
        Log::log_print(ERR, "MiniaudioDevice: failed to start device: %d", result);
        ma_device_uninit(&device_);
        ma_context_uninit(&context_);
        context_open_ = false;
        return false;
    }

    device_open_ = true;
    Log::log_print(INFO, "MiniaudioDevice: opened %dHz %dch (internal: %dHz %dch), state=%d", SAMPLE_RATE, 2,
                   device_.playback.internalSampleRate, device_.playback.internalChannels,
                   ma_device_get_state(&device_));
    return true;
}

void MiniaudioDevice::close() {
    if (device_open_) {
        ma_device_uninit(&device_);
        device_open_ = false;
    }
    if (context_open_) {
        ma_context_uninit(&context_);
        context_open_ = false;
    }
    std::lock_guard lock(mutex_);
    for (auto& ch : channels_) {
        ch.active = false;
        ch.asset.reset();
        ch.stream.reset();
    }
}

void MiniaudioDevice::play(int channel, std::shared_ptr<SoundAsset> asset, bool loop, float volume) {
    if (channel < 0 || channel >= NUM_CHANNELS || !asset)
        return;
    Log::log_print(INFO, "MiniaudioDevice: play ch=%d samples=%zu rate=%d loop=%d vol=%.2f", channel,
                   asset->sample_count(), asset->sample_rate(), loop, volume);
    std::lock_guard lock(mutex_);
    auto& ch = channels_[channel];
    ch.stream.reset();
    ch.asset = std::move(asset);
    ch.position = 0;
    ch.volume = volume;
    ch.loop = loop;
    ch.active = true;
}

void MiniaudioDevice::play_stream(int channel, std::shared_ptr<AudioStream> stream, bool loop, float volume) {
    if (channel < 0 || channel >= NUM_CHANNELS || !stream)
        return;
    Log::log_print(INFO, "MiniaudioDevice: play_stream ch=%d loop=%d vol=%.2f", channel, loop, volume);
    std::lock_guard lock(mutex_);
    auto& ch = channels_[channel];
    ch.asset.reset();
    ch.stream = std::move(stream);
    ch.position = 0;
    ch.volume = volume;
    ch.loop = loop;
    ch.active = true;
}

void MiniaudioDevice::stop(int channel) {
    if (channel < 0 || channel >= NUM_CHANNELS)
        return;
    std::lock_guard lock(mutex_);
    auto& ch = channels_[channel];
    ch.active = false;
    if (ch.stream)
        ch.stream->cancel();
    ch.stream.reset();
    ch.asset.reset();
}

void MiniaudioDevice::set_volume(int channel, float volume) {
    if (channel < 0 || channel >= NUM_CHANNELS)
        return;
    std::lock_guard lock(mutex_);
    channels_[channel].volume = std::clamp(volume, 0.0f, 1.0f);
}

bool MiniaudioDevice::is_playing(int channel) const {
    if (channel < 0 || channel >= NUM_CHANNELS)
        return false;
    std::lock_guard lock(mutex_);
    return channels_[channel].active;
}

void MiniaudioDevice::data_callback(ma_device* device, void* output, const void* /*input*/, ma_uint32 frame_count) {
    auto* self = static_cast<MiniaudioDevice*>(device->pUserData);
    auto* out = static_cast<float*>(output);
    std::memset(out, 0, frame_count * 2 * sizeof(float));
    self->mix(out, static_cast<int>(frame_count));

    // Log first callback and periodic activity
    static int callback_count = 0;
    if (callback_count < 3 || (callback_count % 3000 == 0)) {
        // Check if any audio was mixed (non-silence)
        float peak = 0;
        for (ma_uint32 i = 0; i < frame_count * 2; i++) {
            float v = out[i] < 0 ? -out[i] : out[i];
            if (v > peak)
                peak = v;
        }
        Log::log_print(DEBUG, "MiniaudioDevice: callback #%d frames=%u peak=%.4f", callback_count, frame_count, peak);
    }
    callback_count++;
}

void MiniaudioDevice::mix(float* output, int frames) {
    std::lock_guard lock(mutex_);

    for (auto& ch : channels_) {
        if (!ch.active)
            continue;

        float vol = ch.volume;

        if (ch.stream) {
            // --- Streaming channel (music) ---
            float buf[8192];
            int remaining = frames;
            int out_pos = 0;

            while (remaining > 0) {
                int to_read = std::min(remaining, (int)(sizeof(buf) / sizeof(buf[0]) / 2));
                int got = ch.stream->read_frames(buf, to_read);

                for (int i = 0; i < got * 2; ++i)
                    output[out_pos * 2 + i] += buf[i] * vol;
                out_pos += got;
                remaining -= got;

                if (got < to_read) {
                    if (ch.stream->is_finished() && !ch.stream->is_cancelled()) {
                        ch.active = false;
                        ch.stream.reset();
                    }
                    break;
                }
            }
        }
        else if (ch.asset) {
            // --- Pre-decoded channel (SFX, blips) ---
            const float* src = ch.asset->data();
            size_t total = ch.asset->sample_count();
            uint32_t src_channels = ch.asset->channels();

            for (int f = 0; f < frames; ++f) {
                if (ch.position >= total) {
                    if (ch.loop) {
                        ch.position = 0;
                    }
                    else {
                        ch.active = false;
                        ch.asset.reset();
                        break;
                    }
                }

                float left, right;
                if (src_channels == 2) {
                    left = src[ch.position];
                    right = src[ch.position + 1];
                    ch.position += 2;
                }
                else {
                    left = right = src[ch.position];
                    ch.position += 1;
                }

                output[f * 2] += left * vol;
                output[f * 2 + 1] += right * vol;
            }
        }
    }

    // Clamp to prevent clipping
    for (int i = 0; i < frames * 2; ++i) {
        output[i] = std::clamp(output[i], -1.0f, 1.0f);
    }
}

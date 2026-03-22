#include "audio/SDLAudioDevice.h"

#include "utils/Log.h"

#include <algorithm>
#include <cstring>

SDLAudioDevice::~SDLAudioDevice() {
    close();
}

bool SDLAudioDevice::open() {
    SDL_AudioSpec desired{};
    desired.freq = SAMPLE_RATE;
    desired.format = AUDIO_F32SYS;
    desired.channels = 2;
    desired.samples = BUFFER_SAMPLES;
    desired.callback = audio_callback;
    desired.userdata = this;

    SDL_AudioSpec obtained{};
    device_id_ = SDL_OpenAudioDevice(nullptr, 0, &desired, &obtained, 0);
    if (device_id_ == 0) {
        Log::log_print(ERR, "SDLAudioDevice: failed to open audio: %s", SDL_GetError());
        return false;
    }

    Log::log_print(INFO, "SDLAudioDevice: opened %dHz %dch, buffer %d samples", obtained.freq, obtained.channels,
                   obtained.samples);

    // Unpause — SDL devices start paused
    SDL_PauseAudioDevice(device_id_, 0);
    return true;
}

void SDLAudioDevice::close() {
    if (device_id_ != 0) {
        SDL_CloseAudioDevice(device_id_);
        device_id_ = 0;
    }
    std::lock_guard lock(mutex_);
    for (auto& ch : channels_) {
        ch.active = false;
        ch.asset.reset();
    }
}

std::vector<SDLAudioDevice::ChannelInfo> SDLAudioDevice::channel_snapshot() const {
    std::lock_guard lock(mutex_);
    std::vector<ChannelInfo> info;
    for (int i = 0; i < NUM_CHANNELS; ++i) {
        const auto& ch = channels_[i];
        if (!ch.active)
            continue;
        ChannelInfo ci{};
        ci.id = i;
        ci.active = ch.active;
        ci.is_stream = (ch.stream != nullptr);
        ci.stream_ready = ch.stream ? ch.stream->is_ready() : false;
        ci.stream_finished = ch.stream ? ch.stream->is_finished() : false;
        ci.stream_looping = ch.stream ? ch.stream->is_looping() : ch.loop;
        ci.loop_start = ch.stream ? ch.stream->loop_start() : 0;
        ci.loop_end = ch.stream ? ch.stream->loop_end() : 0;
        ci.volume = ch.volume;
        ci.ring_available = ch.stream ? ch.stream->buffered_samples() : 0;
        info.push_back(ci);
    }
    return info;
}

void SDLAudioDevice::play(int channel, std::shared_ptr<SoundAsset> asset, bool loop, float volume) {
    if (channel < 0 || channel >= NUM_CHANNELS || !asset)
        return;
    std::lock_guard lock(mutex_);
    auto& ch = channels_[channel];
    ch.stream.reset(); // Clear any active stream
    ch.asset = std::move(asset);
    ch.position = 0;
    ch.volume = volume;
    ch.loop = loop;
    ch.active = true;
}

void SDLAudioDevice::play_stream(int channel, std::shared_ptr<AudioStream> stream, bool loop, float volume) {
    if (channel < 0 || channel >= NUM_CHANNELS || !stream)
        return;
    std::lock_guard lock(mutex_);
    auto& ch = channels_[channel];
    ch.asset.reset(); // Clear any pre-decoded asset
    ch.stream = std::move(stream);
    ch.position = 0;
    ch.volume = volume;
    ch.loop = loop;
    ch.active = true;
}

void SDLAudioDevice::stop(int channel) {
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

void SDLAudioDevice::set_volume(int channel, float volume) {
    if (channel < 0 || channel >= NUM_CHANNELS)
        return;
    std::lock_guard lock(mutex_);
    channels_[channel].volume = std::clamp(volume, 0.0f, 1.0f);
}

bool SDLAudioDevice::is_playing(int channel) const {
    if (channel < 0 || channel >= NUM_CHANNELS)
        return false;
    std::lock_guard lock(mutex_);
    return channels_[channel].active;
}

void SDLAudioDevice::audio_callback(void* userdata, Uint8* stream, int len) {
    auto* device = static_cast<SDLAudioDevice*>(userdata);
    int frames = len / (2 * sizeof(float)); // stereo float32
    auto* output = reinterpret_cast<float*>(stream);
    std::memset(output, 0, len);
    device->mix(output, frames);
}

void SDLAudioDevice::mix(float* output, int frames) {
    std::lock_guard lock(mutex_);

    for (auto& ch : channels_) {
        if (!ch.active)
            continue;

        float vol = ch.volume;

        if (ch.stream) {
            // --- Streaming channel (music) ---
            float buf[8192]; // temp buffer for stream read
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
                        // Stream fully decoded and not looping → mark channel inactive
                        ch.active = false;
                        ch.stream.reset();
                    }
                    break; // Silence for the rest (underrun or finished)
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

    // Clamp to [-1, 1] to prevent clipping distortion
    for (int i = 0; i < frames * 2; ++i) {
        output[i] = std::clamp(output[i], -1.0f, 1.0f);
    }
}

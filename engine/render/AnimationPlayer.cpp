#include "render/AnimationPlayer.h"

#include "utils/Log.h"

void AnimationPlayer::load(std::shared_ptr<ImageAsset> asset, bool loop) {
    current_asset = std::move(asset);
    frame_index = 0;
    elapsed_ms = 0;
    looping = loop;
    done = false;

    if (current_asset) {
        Log::log_print(DEBUG, "AnimPlayer: loaded %d frames, loop=%d, path=%s", current_asset->frame_count(), loop,
                       current_asset->path().c_str());
        if (current_asset->frame_count() > 0) {
            Log::log_print(DEBUG, "AnimPlayer: frame0 duration=%dms, size=%dx%d", current_asset->frame(0).duration_ms,
                           current_asset->frame(0).width, current_asset->frame(0).height);
        }
    }
}

void AnimationPlayer::clear() {
    current_asset = nullptr;
    frame_index = 0;
    elapsed_ms = 0;
    done = false;
}

void AnimationPlayer::tick(int delta_ms) {
    if (!current_asset || current_asset->frame_count() == 0 || done) {
        return;
    }

    // Single-frame asset: for looping, just hold forever.
    // For one-shot, hold for the frame's duration (min 100ms) then mark done.
    if (current_asset->frame_count() == 1) {
        if (!looping) {
            elapsed_ms += delta_ms;
            int duration = current_asset->frame(0).duration_ms;
            if (duration <= 0)
                duration = 100;
            if (elapsed_ms >= duration) {
                done = true;
            }
        }
        return;
    }

    elapsed_ms += delta_ms;

    const ImageFrame& frame = current_asset->frame(frame_index);
    int duration = frame.duration_ms > 0 ? frame.duration_ms : 100; // default 100ms

    while (elapsed_ms >= duration) {
        elapsed_ms -= duration;
        int old_frame = frame_index;
        frame_index++;

        if (frame_index >= current_asset->frame_count()) {
            if (looping) {
                frame_index = 0;
                Log::log_print(VERBOSE, "AnimPlayer: looped back to frame 0 (was %d/%d) path=%s", old_frame,
                               current_asset->frame_count(), current_asset->path().c_str());
            }
            else {
                frame_index = current_asset->frame_count() - 1;
                done = true;
                Log::log_print(DEBUG, "AnimPlayer: finished one-shot at frame %d", frame_index);
                return;
            }
        }

        duration = current_asset->frame(frame_index).duration_ms;
        if (duration <= 0)
            duration = 100;
    }
}

bool AnimationPlayer::has_frame() const {
    return current_asset && current_asset->frame_count() > 0;
}

const ImageFrame* AnimationPlayer::current_frame() const {
    if (!current_asset || current_asset->frame_count() == 0) {
        return nullptr;
    }
    return &current_asset->frame(frame_index);
}

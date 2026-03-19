#include "ao/game/AOEmotePlayer.h"

#include "utils/Log.h"

void AOEmotePlayer::start(AssetLibrary& assets, const std::string& character,
                          const std::string& emote, const std::string& pre_emote,
                          EmoteMod emote_mod) {
    std::string base = "characters/" + character + "/";

    // Pre-animation: direct name (e.g. "objecting"), no prefix
    std::shared_ptr<ImageAsset> preanim_asset;
    if (!pre_emote.empty() && pre_emote != "-") {
        preanim_asset = assets.image(base + pre_emote);
    }

    // Idle (a-emote): try (a){emote}, fall back to {emote}
    auto idle_asset = assets.image(base + "(a)" + emote);
    if (!idle_asset) {
        idle_asset = assets.image(base + emote);
    }

    // Talking (b-emote): try (b){emote}, fall back to {emote}, then idle
    auto talk_asset = assets.image(base + "(b)" + emote);
    if (!talk_asset) {
        talk_asset = assets.image(base + emote);
    }
    if (!talk_asset) {
        talk_asset = idle_asset;
    }

    idle.load(idle_asset, true);
    talk.load(talk_asset, true);
    preanim.clear();

    switch (emote_mod) {
        case EmoteMod::PREANIM:
        case EmoteMod::PREANIM_ZOOM:
            if (preanim_asset && preanim_asset->frame_count() > 0) {
                preanim.load(preanim_asset, false);
                current_state = State::PREANIM;
            } else {
                current_state = State::TALKING;
            }
            break;
        case EmoteMod::ZOOM:
            current_state = State::TALKING;
            break;
        case EmoteMod::IDLE:
        default:
            current_state = State::IDLE;
            break;
    }

    Log::log_print(DEBUG, "Emote: char=%s emote=%s pre=%s state=%d",
                   character.c_str(), emote.c_str(), pre_emote.c_str(),
                   static_cast<int>(current_state));
}

void AOEmotePlayer::tick(int delta_ms) {
    switch (current_state) {
        case State::PREANIM:
            preanim.tick(delta_ms);
            if (preanim.finished()) {
                current_state = State::TALKING;
            }
            break;
        case State::TALKING:
            talk.tick(delta_ms);
            // TODO: transition to IDLE when text finishes scrolling
            break;
        case State::IDLE:
            idle.tick(delta_ms);
            break;
        case State::NONE:
            break;
    }
}

const ImageFrame* AOEmotePlayer::current_frame() const {
    switch (current_state) {
        case State::PREANIM: return preanim.current_frame();
        case State::TALKING: return talk.current_frame();
        case State::IDLE:    return idle.current_frame();
        default:             return nullptr;
    }
}

int AOEmotePlayer::current_frame_index() const {
    switch (current_state) {
        case State::PREANIM: return preanim.current_frame_index();
        case State::TALKING: return talk.current_frame_index();
        case State::IDLE:    return idle.current_frame_index();
        default:             return 0;
    }
}

static const std::shared_ptr<ImageAsset> null_asset;

const std::shared_ptr<ImageAsset>& AOEmotePlayer::asset() const {
    switch (current_state) {
        case State::PREANIM: return preanim.asset();
        case State::TALKING: return talk.asset();
        case State::IDLE:    return idle.asset();
        default:             return null_asset;
    }
}

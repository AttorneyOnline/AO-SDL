#include "ao/game/AOEmotePlayer.h"

#include "utils/Log.h"

void AOEmotePlayer::start(AOAssetLibrary& ao_assets, const std::string& character, const std::string& emote,
                          const std::string& pre_emote, EmoteMod emote_mod) {
    // Preanim: direct name, no prefix
    std::shared_ptr<ImageAsset> preanim_asset;
    if (!pre_emote.empty() && pre_emote != "-") {
        preanim_asset = ao_assets.character_emote(character, pre_emote, "");
    }

    // Idle: (a) prefix
    auto idle_asset = ao_assets.character_emote(character, emote, "(a)");

    // Talking: (b) prefix, fall back to idle
    auto talk_asset = ao_assets.character_emote(character, emote, "(b)");
    if (!talk_asset)
        talk_asset = idle_asset;

    idle.load(idle_asset, true);
    talk.load(talk_asset, true);
    preanim.clear();

    switch (emote_mod) {
    case EmoteMod::PREANIM:
    case EmoteMod::PREANIM_ZOOM:
        if (preanim_asset && preanim_asset->frame_count() > 0) {
            preanim.load(preanim_asset, false);
            current_state = State::PREANIM;
        }
        else {
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

    Log::log_print(DEBUG, "Emote: char=%s emote=%s pre=%s state=%d", character.c_str(), emote.c_str(),
                   pre_emote.c_str(), static_cast<int>(current_state));
}

void AOEmotePlayer::transition_to_idle() {
    if (current_state == State::TALKING) {
        current_state = State::IDLE;
    }
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
    case State::PREANIM:
        return preanim.current_frame();
    case State::TALKING:
        return talk.current_frame();
    case State::IDLE:
        return idle.current_frame();
    default:
        return nullptr;
    }
}

int AOEmotePlayer::current_frame_index() const {
    switch (current_state) {
    case State::PREANIM:
        return preanim.current_frame_index();
    case State::TALKING:
        return talk.current_frame_index();
    case State::IDLE:
        return idle.current_frame_index();
    default:
        return 0;
    }
}

static const std::shared_ptr<ImageAsset> null_asset;

const std::shared_ptr<ImageAsset>& AOEmotePlayer::asset() const {
    switch (current_state) {
    case State::PREANIM:
        return preanim.asset();
    case State::TALKING:
        return talk.asset();
    case State::IDLE:
        return idle.asset();
    default:
        return null_asset;
    }
}

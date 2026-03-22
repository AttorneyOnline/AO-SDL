#include "ao/game/AOEmotePlayer.h"

#include "utils/Log.h"

void AOEmotePlayer::start(AOAssetLibrary& ao_assets, const std::string& character, const std::string& emote,
                          const std::string& pre_emote, EmoteMod emote_mod) {
    // Store for retry
    character_ = character;
    emote_ = emote;
    pre_emote_ = pre_emote;
    emote_mod_ = emote_mod;
    retry_count_ = 0;
    fallback_prefetched_ = false;

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

    // If no assets loaded yet (HTTP download pending), mark for retry
    needs_retry_ = !idle_asset && !talk_asset;
    if (needs_retry_ && !emote.empty())
        Log::log_print(DEBUG, "Emote: needs retry for %s/%s (HTTP pending)", character.c_str(), emote.c_str());

    idle.load(idle_asset, true);
    talk.load(talk_asset, true);
    preanim.clear();

    // Play preanim if the asset exists AND emote_mod requests it.
    // The sender decides whether to use preanim via the "Pre" checkbox
    // (emote_mod PREANIM/PREANIM_ZOOM). The presenter handles blocking vs
    // concurrent based on the immediate flag.
    bool wants_preanim = (emote_mod == EmoteMod::PREANIM || emote_mod == EmoteMod::PREANIM_ZOOM);
    if (wants_preanim && preanim_asset && preanim_asset->frame_count() > 0) {
        preanim.load(preanim_asset, false);
        current_state = State::PREANIM;
    }
    else {
        current_state = State::TALKING;
    }

    Log::log_print(VERBOSE, "Emote: char=%s emote=%s pre=%s state=%d", character.c_str(), emote.c_str(),
                   pre_emote.c_str(), static_cast<int>(current_state));
}

bool AOEmotePlayer::retry_load(AOAssetLibrary& ao_assets) {
    if (!needs_retry_)
        return false;

    // If server-advertised extensions all failed, re-prefetch with default
    // extensions so formats the server didn't list can still be found.
    if (retry_count_++ > 5 && !fallback_prefetched_) {
        fallback_prefetched_ = true;
        ao_assets.prefetch_character(character_, emote_, pre_emote_);
    }

    auto idle_asset = ao_assets.character_emote(character_, emote_, "(a)");
    if (!idle_asset)
        return false; // still not available

    auto talk_asset = ao_assets.character_emote(character_, emote_, "(b)");
    if (!talk_asset)
        talk_asset = idle_asset;

    idle.load(idle_asset, true);
    talk.load(talk_asset, true);
    needs_retry_ = false;

    Log::log_print(DEBUG, "Emote: retry loaded %s/%s", character_.c_str(), emote_.c_str());
    return true;
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

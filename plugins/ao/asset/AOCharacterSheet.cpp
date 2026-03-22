#include "ao/asset/AOCharacterSheet.h"

#include "utils/Log.h"

#include <algorithm>
#include <cstdlib>

const std::vector<std::pair<int, std::string>> AOCharacterSheet::empty_frame_sfx_;
const std::vector<int> AOCharacterSheet::empty_frame_ints_;

const std::unordered_map<std::string, std::string>* AOCharacterSheet::find_section(const IniDocument& ini,
                                                                                   const std::string& name) {
    auto it = ini.find(name);
    if (it != ini.end())
        return &it->second;
    // Try lowercase
    std::string lower = name;
    std::transform(lower.begin(), lower.end(), lower.begin(), ::tolower);
    it = ini.find(lower);
    if (it != ini.end())
        return &it->second;
    return nullptr;
}

std::optional<AOCharacterSheet> AOCharacterSheet::load(AssetLibrary& assets, const std::string& character) {
    auto ini = assets.config("characters/" + character + "/char.ini");
    if (!ini) {
        Log::log_print(VERBOSE, "AOCharacterSheet: char.ini not found for '%s'", character.c_str());
        return std::nullopt;
    }

    AOCharacterSheet sheet;
    sheet.name_ = character;
    sheet.showname_ = character;

    sheet.parse_options(*ini);
    sheet.parse_emotes(*ini);
    sheet.parse_sounds(*ini);
    sheet.parse_time(*ini);
    sheet.parse_frame_effects(*ini);

    return sheet;
}

void AOCharacterSheet::parse_options(const IniDocument& ini) {
    auto* kv = find_section(ini, "Options");
    if (!kv)
        return;

    auto get = [&](const std::string& key) -> const std::string* {
        auto it = kv->find(key);
        if (it != kv->end() && !it->second.empty())
            return &it->second;
        return nullptr;
    };

    if (auto* v = get("showname"))
        showname_ = *v;
    if (auto* v = get("side"))
        side_ = *v;
    if (auto* v = get("blips"))
        blips_ = *v;
    if (auto* v = get("effects"))
        effects_ = *v;
    if (auto* v = get("chat"))
        chat_ = *v;
}

void AOCharacterSheet::parse_emotes(const IniDocument& ini) {
    auto* kv = find_section(ini, "Emotions");
    if (!kv)
        return;

    int count = 0;
    auto it = kv->find("number");
    if (it != kv->end())
        count = std::atoi(it->second.c_str());

    for (int i = 1; i <= count; i++) {
        auto it = kv->find(std::to_string(i));
        if (it == kv->end())
            continue;

        // Format: Comment#PreAnim#AnimName#Mod#DeskMod
        AOEmoteEntry entry;
        const std::string& line = it->second;

        size_t prev = 0;
        size_t pos = line.find('#');
        entry.comment = line.substr(prev, pos - prev);
        if (pos == std::string::npos) {
            emotes_.push_back(entry);
            continue;
        }

        prev = pos + 1;
        pos = line.find('#', prev);
        entry.pre_anim = line.substr(prev, pos == std::string::npos ? pos : pos - prev);
        if (pos == std::string::npos) {
            emotes_.push_back(entry);
            continue;
        }

        prev = pos + 1;
        pos = line.find('#', prev);
        entry.anim_name = line.substr(prev, pos == std::string::npos ? pos : pos - prev);
        if (pos == std::string::npos) {
            emotes_.push_back(entry);
            continue;
        }

        prev = pos + 1;
        pos = line.find('#', prev);
        entry.mod = std::atoi(line.substr(prev, pos == std::string::npos ? pos : pos - prev).c_str());
        if (pos == std::string::npos) {
            emotes_.push_back(entry);
            continue;
        }

        prev = pos + 1;
        entry.desk_mod = std::atoi(line.substr(prev).c_str());

        emotes_.push_back(entry);
    }
}

void AOCharacterSheet::parse_sounds(const IniDocument& ini) {
    // [SoundN] — SFX name per emote
    auto* sn = find_section(ini, "SoundN");
    if (sn) {
        for (auto& [key, val] : *sn) {
            int idx = std::atoi(key.c_str()) - 1; // 1-indexed
            if (idx >= 0 && idx < (int)emotes_.size() && val != "0" && val != "1")
                emotes_[idx].sfx_name = val;
        }
    }

    // [SoundT] — SFX delay per emote
    auto* st = find_section(ini, "SoundT");
    if (st) {
        for (auto& [key, val] : *st) {
            int idx = std::atoi(key.c_str()) - 1;
            if (idx >= 0 && idx < (int)emotes_.size())
                emotes_[idx].sfx_delay = std::atoi(val.c_str());
        }
    }

    // [SoundL] — looping SFX flag (if present)
    auto* sl = find_section(ini, "SoundL");
    if (sl) {
        for (auto& [key, val] : *sl) {
            int idx = std::atoi(key.c_str()) - 1;
            if (idx >= 0 && idx < (int)emotes_.size())
                emotes_[idx].sfx_looping = (val == "1");
        }
    }
}

void AOCharacterSheet::parse_time(const IniDocument& ini) {
    auto* kv = find_section(ini, "Time");
    if (!kv)
        return;
    for (auto& [key, val] : *kv) {
        int ms = std::atoi(val.c_str());
        if (ms > 0)
            preanim_durations_[key] = ms;
    }
}

void AOCharacterSheet::parse_frame_effects(const IniDocument& ini) {
    // Scan all sections for *_FrameSFX, *_FrameScreenshake, *_FrameRealization
    for (auto& [section, kv] : ini) {
        size_t pos;

        pos = section.find("_FrameSFX");
        if (pos != std::string::npos && pos > 0) {
            std::string anim = section.substr(0, pos);
            auto& vec = frame_sfx_[anim];
            for (auto& [key, val] : kv) {
                int frame = std::atoi(key.c_str());
                vec.emplace_back(frame, val);
            }
            std::sort(vec.begin(), vec.end());
            continue;
        }

        pos = section.find("_FrameScreenshake");
        if (pos != std::string::npos && pos > 0) {
            std::string anim = section.substr(0, pos);
            auto& vec = frame_screenshake_[anim];
            for (auto& [key, val] : kv) {
                vec.push_back(std::atoi(key.c_str()));
            }
            std::sort(vec.begin(), vec.end());
            continue;
        }

        pos = section.find("_FrameRealization");
        if (pos != std::string::npos && pos > 0) {
            std::string anim = section.substr(0, pos);
            auto& vec = frame_realization_[anim];
            for (auto& [key, val] : kv) {
                vec.push_back(std::atoi(key.c_str()));
            }
            std::sort(vec.begin(), vec.end());
            continue;
        }
    }
}

int AOCharacterSheet::preanim_duration_ms(const std::string& pre_anim) const {
    auto it = preanim_durations_.find(pre_anim);
    return it != preanim_durations_.end() ? it->second : 0;
}

const std::vector<std::pair<int, std::string>>& AOCharacterSheet::frame_sfx(const std::string& anim_name) const {
    auto it = frame_sfx_.find(anim_name);
    return it != frame_sfx_.end() ? it->second : empty_frame_sfx_;
}

const std::vector<int>& AOCharacterSheet::frame_screenshake(const std::string& anim_name) const {
    auto it = frame_screenshake_.find(anim_name);
    return it != frame_screenshake_.end() ? it->second : empty_frame_ints_;
}

const std::vector<int>& AOCharacterSheet::frame_realization(const std::string& anim_name) const {
    auto it = frame_realization_.find(anim_name);
    return it != frame_realization_.end() ? it->second : empty_frame_ints_;
}

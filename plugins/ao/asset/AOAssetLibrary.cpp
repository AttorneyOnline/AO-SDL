#include "ao/asset/AOAssetLibrary.h"

#include "asset/ImageDecoder.h"
#include "asset/MediaManager.h"
#include "asset/MountManager.h"
#include "utils/Log.h"

#include <algorithm>
#include <cctype>
#include <cstdio>

AOAssetLibrary::AOAssetLibrary(AssetLibrary& assets, const std::string& theme) : assets(assets), active_theme(theme) {
}

// ---- Static helpers --------------------------------------------------------

std::string AOAssetLibrary::bg_filename(const std::string& pos) {
    if (pos == "def")
        return "defenseempty";
    if (pos == "pro")
        return "prosecutorempty";
    if (pos == "wit")
        return "witnessempty";
    if (pos == "jud")
        return "judgestand";
    if (pos == "hld")
        return "helperstand";
    if (pos == "hlp")
        return "prohelperstand";
    if (pos == "jur")
        return "jurystand";
    if (pos == "sea")
        return "seancestand";
    return pos;
}

std::string AOAssetLibrary::desk_filename(const std::string& pos) {
    if (pos == "def")
        return "defensedesk";
    if (pos == "pro")
        return "prosecutiondesk";
    if (pos == "wit")
        return "stand";
    if (pos == "jud")
        return "judgedesk";
    if (pos == "hld")
        return "helperdesk";
    if (pos == "hlp")
        return "prohelperdesk";
    if (pos == "jur")
        return "jurydesk";
    if (pos == "sea")
        return "seancedesk";
    return pos + "_overlay";
}

std::string AOAssetLibrary::normalize_font_name(const std::string& name) {
    std::string out = name;
    // Trim
    while (!out.empty() && std::isspace((unsigned char)out.back()))
        out.pop_back();
    while (!out.empty() && std::isspace((unsigned char)out.front()))
        out.erase(out.begin());
    // Lowercase + spaces→hyphens
    for (auto& c : out) {
        if (c == ' ')
            c = '-';
        c = (char)std::tolower((unsigned char)c);
    }
    return out;
}

// ---- Config caching --------------------------------------------------------

void AOAssetLibrary::ensure_configs() {
    if (configs_loaded)
        return;
    configs_loaded = true;

    cached_design = assets.config("themes/" + active_theme + "/courtroom_design.ini");
    if (!cached_design)
        cached_design = assets.config("themes/default/courtroom_design.ini");

    cached_fonts = assets.config("themes/" + active_theme + "/courtroom_fonts.ini");
    if (!cached_fonts)
        cached_fonts = assets.config("themes/default/courtroom_fonts.ini");

    cached_chat_config = assets.config("themes/" + active_theme + "/chat_config.ini");
    if (!cached_chat_config)
        cached_chat_config = assets.config("themes/default/chat_config.ini");
}

// ---- Character sprites -----------------------------------------------------

std::shared_ptr<ImageAsset> AOAssetLibrary::character_emote(const std::string& character, const std::string& emote,
                                                            const std::string& prefix) {
    std::string base = "characters/" + character + "/";
    auto result = assets.image(base + prefix + emote);
    if (result && !prefix.empty()) {
        // Prefixed version found — release the bare name variant from HTTP cache
        // since it was also prefetched but won't be decoded.
        auto& mm = MediaManager::instance().mounts_ref();
        for (const auto& ext : supported_image_extensions())
            mm.release_http(base + emote + "." + ext);
    }
    if (!result && !prefix.empty()) {
        result = assets.image(base + emote);
    }
    return result;
}

std::shared_ptr<ImageAsset> AOAssetLibrary::character_icon(const std::string& character) {
    return assets.image("characters/" + character + "/char_icon");
}

std::shared_ptr<ImageAsset> AOAssetLibrary::emote_icon(const std::string& character, int emote_index) {
    std::string path = "characters/" + character + "/emotions/button" + std::to_string(emote_index + 1) + "_off";
    return assets.image(path);
}

std::optional<AOCharacterSheet> AOAssetLibrary::character_sheet(const std::string& character) {
    return AOCharacterSheet::load(assets, character);
}

// ---- Background ------------------------------------------------------------

std::shared_ptr<ImageAsset> AOAssetLibrary::background(const std::string& name, const std::string& position) {
    std::string legacy = bg_filename(position);
    auto result = assets.image("background/" + name + "/" + legacy);
    // Modern naming fallback
    if (!result && legacy != position) {
        result = assets.image("background/" + name + "/" + position);
    }
    // Default background fallback
    if (!result && name != "default") {
        result = assets.image("background/default/" + bg_filename(position));
    }
    return result;
}

std::shared_ptr<ImageAsset> AOAssetLibrary::desk_overlay(const std::string& name, const std::string& position) {
    std::string filename = desk_filename(position);
    auto result = assets.image("background/" + name + "/" + filename);
    if (!result && name != "default") {
        result = assets.image("background/default/" + filename);
    }
    return result;
}

void AOAssetLibrary::prefetch_background(const std::string& name, const std::string& position, int priority) {
    std::string base = "background/" + name + "/";
    std::string legacy = bg_filename(position);
    // Prefetch both legacy name (e.g. "prosecutorempty") and modern name (e.g. "pro")
    assets.prefetch_image(base + legacy, 3, priority);
    if (legacy != position)
        assets.prefetch_image(base + position, 3, priority);
}

// ---- Theme / UI ------------------------------------------------------------

std::shared_ptr<ImageAsset> AOAssetLibrary::theme_image(const std::string& element) {
    auto result = assets.image("themes/" + active_theme + "/" + element);
    if (!result && active_theme != "default") {
        result = assets.image("themes/default/" + element);
    }
    return result;
}

std::optional<IniDocument> AOAssetLibrary::theme_config(const std::string& filename) {
    auto result = assets.config("themes/" + active_theme + "/" + filename);
    if (!result && active_theme != "default") {
        result = assets.config("themes/default/" + filename);
    }
    return result;
}

AORect AOAssetLibrary::design_rect(const std::string& key) {
    ensure_configs();
    AORect r;
    if (cached_design) {
        auto it = cached_design->find("");
        if (it != cached_design->end()) {
            auto val = it->second.find(key);
            if (val != it->second.end()) {
                std::sscanf(val->second.c_str(), "%d, %d, %d, %d", &r.x, &r.y, &r.w, &r.h);
            }
        }
    }
    return r;
}

std::string AOAssetLibrary::design_value(const std::string& key) {
    ensure_configs();
    if (cached_design) {
        auto it = cached_design->find("");
        if (it != cached_design->end()) {
            auto val = it->second.find(key);
            if (val != it->second.end())
                return val->second;
        }
    }
    return {};
}

AOFontSpec AOAssetLibrary::message_font_spec() {
    ensure_configs();
    AOFontSpec spec;
    spec.name = "arial";
    spec.size_pt = 10;
    spec.sharp = true;

    if (cached_fonts) {
        auto it = cached_fonts->find("");
        if (it != cached_fonts->end()) {
            auto fn = it->second.find("message_font");
            if (fn != it->second.end()) {
                spec.name = normalize_font_name(fn->second);
            }
            auto fs = it->second.find("message");
            if (fs != it->second.end()) {
                spec.size_pt = std::atoi(fs->second.c_str());
                if (spec.size_pt <= 0)
                    spec.size_pt = 10;
            }
            auto sh = it->second.find("message_sharp");
            if (sh != it->second.end()) {
                spec.sharp = (sh->second == "1");
            }
        }
    }

    spec.size_px = (spec.size_pt * 4 + 2) / 3; // pt→px at 96 DPI
    return spec;
}

AOFontSpec AOAssetLibrary::showname_font_spec() {
    ensure_configs();
    AOFontSpec spec;
    // Default to message font if showname font isn't specified
    spec = message_font_spec();

    if (cached_fonts) {
        auto it = cached_fonts->find("");
        if (it != cached_fonts->end()) {
            auto fn = it->second.find("showname_font");
            if (fn != it->second.end()) {
                spec.name = normalize_font_name(fn->second);
            }
            auto fs = it->second.find("showname");
            if (fs != it->second.end()) {
                spec.size_pt = std::atoi(fs->second.c_str());
                if (spec.size_pt <= 0)
                    spec.size_pt = 10;
                spec.size_px = (spec.size_pt * 4 + 2) / 3;
            }
            auto sh = it->second.find("showname_sharp");
            if (sh != it->second.end()) {
                spec.sharp = (sh->second == "1");
            }
        }
    }

    return spec;
}

std::vector<AOTextColorDef> AOAssetLibrary::text_colors() {
    ensure_configs();

    // Defaults from the legacy client
    std::vector<AOTextColorDef> colors;
    colors.push_back({247, 247, 247, true});  // 0: White
    colors.push_back({0, 247, 0, true});      // 1: Green
    colors.push_back({247, 0, 57, true});     // 2: Red
    colors.push_back({247, 115, 57, false});  // 3: Orange
    colors.push_back({107, 198, 247, false}); // 4: Blue
    colors.push_back({247, 247, 0, true});    // 5: Yellow
    colors.push_back({247, 115, 247, true});  // 6: Magenta
    colors.push_back({128, 247, 247, true});  // 7: Cyan
    colors.push_back({160, 181, 205, true});  // 8: Gray

    if (cached_chat_config) {
        auto it = cached_chat_config->find("");
        if (it != cached_chat_config->end()) {
            for (int i = 0; i < (int)colors.size(); i++) {
                std::string key = "c" + std::to_string(i);
                auto val = it->second.find(key);
                if (val != it->second.end()) {
                    int r, g, b;
                    if (std::sscanf(val->second.c_str(), "%d, %d, %d", &r, &g, &b) == 3) {
                        colors[i].r = (uint8_t)r;
                        colors[i].g = (uint8_t)g;
                        colors[i].b = (uint8_t)b;
                    }
                }
                auto talk = it->second.find(key + "_talking");
                if (talk != it->second.end()) {
                    colors[i].talking = (talk->second != "0");
                }
            }
        }
    }

    return colors;
}

std::optional<std::vector<uint8_t>> AOAssetLibrary::find_font(const std::string& normalized_name) {
    // Build candidate filenames: original, underscored, and _Regular variants.
    // Qt matches font family names to files via its font database; we don't have
    // that, so we try common naming conventions used by AO theme fonts.
    std::string underscore_name = normalized_name;
    for (auto& c : underscore_name)
        if (c == '-') c = '_';

    std::vector<std::string> candidates = {
        normalized_name + ".ttf",
        underscore_name + ".ttf",
        normalized_name + "-regular.ttf",
        underscore_name + "_regular.ttf",
        normalized_name + "_Regular.ttf",
        underscore_name + "_Regular.ttf",
    };

    // Search: active theme → default → known font-shipping themes → global fonts
    std::vector<std::string> dirs = {
        "themes/" + active_theme + "/",
    };
    if (active_theme != "default")
        dirs.push_back("themes/default/");
    dirs.push_back("themes/AceAttorney DS/");
    dirs.push_back("themes/AceAttorney2x/");
    dirs.push_back("themes/AceAttorney 2x/");
    dirs.push_back("fonts/");

    for (const auto& dir : dirs) {
        for (const auto& filename : candidates) {
            auto result = assets.raw(dir + filename);
            if (result) {
                Log::log_print(DEBUG, "AOAssetLibrary: found font '%s%s'", dir.c_str(), filename.c_str());
                return result;
            }
        }
    }

    Log::log_print(WARNING, "AOAssetLibrary: font '%s' not found", normalized_name.c_str());
    return std::nullopt;
}

void AOAssetLibrary::prefetch_character(const std::string& character, const std::string& emote,
                                         const std::string& pre_emote, int priority) {
    std::string base = "characters/" + character + "/";
    assets.prefetch_image(base + "(a)" + emote, 1, priority);
    assets.prefetch_image(base + "(b)" + emote, 1, priority);
    assets.prefetch_image(base + emote, 1, priority); // bare name fallback
    if (!pre_emote.empty() && pre_emote != "-")
        assets.prefetch_image(base + pre_emote, 1, priority);
    assets.prefetch_config(base + "char.ini");
}

void AOAssetLibrary::prefetch_own_character(const std::string& character) {
    // char.ini is fetched synchronously by the config path, so it's
    // already available. Use it to prefetch all emote icons and sprites.
    auto sheet = character_sheet(character);
    if (!sheet)
        return;

    std::string base = "characters/" + character + "/";
    int count = sheet->emote_count();
    for (int i = 0; i < count; i++) {
        // Emote button icons (2=EMOTIONS type)
        assets.prefetch_image(base + "emotions/button" + std::to_string(i + 1) + "_off", 2, 2);
        // Emote sprites (1=EMOTE type)
        const auto& emo = sheet->emote(i);
        assets.prefetch_image(base + "(a)" + emo.anim_name, 1, 2);
        assets.prefetch_image(base + "(b)" + emo.anim_name, 1, 2);
        assets.prefetch_image(base + emo.anim_name, 1, 2); // bare name fallback
        if (!emo.pre_anim.empty() && emo.pre_anim != "-")
            assets.prefetch_image(base + emo.pre_anim, 1, 2);
    }
}

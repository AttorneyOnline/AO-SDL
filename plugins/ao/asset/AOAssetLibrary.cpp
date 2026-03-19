#include "ao/asset/AOAssetLibrary.h"

#include "utils/Log.h"

#include <algorithm>
#include <cctype>
#include <cstdio>

AOAssetLibrary::AOAssetLibrary(AssetLibrary& assets, const std::string& theme)
    : assets(assets), active_theme(theme) {}

// ---- Static helpers --------------------------------------------------------

std::string AOAssetLibrary::bg_filename(const std::string& pos) {
    if (pos == "def") return "defenseempty";
    if (pos == "pro") return "prosecutorempty";
    if (pos == "wit") return "witnessempty";
    if (pos == "jud") return "judgestand";
    if (pos == "hld") return "helperstand";
    if (pos == "hlp") return "prohelperstand";
    if (pos == "jur") return "jurystand";
    if (pos == "sea") return "seancestand";
    return pos;
}

std::string AOAssetLibrary::desk_filename(const std::string& pos) {
    if (pos == "def") return "defensedesk";
    if (pos == "pro") return "prosecutiondesk";
    if (pos == "wit") return "stand";
    if (pos == "jud") return "judgedesk";
    if (pos == "hld") return "helperdesk";
    if (pos == "hlp") return "prohelperdesk";
    if (pos == "jur") return "jurydesk";
    if (pos == "sea") return "seancedesk";
    return pos + "_overlay";
}

std::string AOAssetLibrary::normalize_font_name(const std::string& name) {
    std::string out = name;
    // Trim
    while (!out.empty() && std::isspace((unsigned char)out.back())) out.pop_back();
    while (!out.empty() && std::isspace((unsigned char)out.front())) out.erase(out.begin());
    // Lowercase + spaces→hyphens
    for (auto& c : out) {
        if (c == ' ') c = '-';
        c = (char)std::tolower((unsigned char)c);
    }
    return out;
}

// ---- Config caching --------------------------------------------------------

void AOAssetLibrary::ensure_configs() {
    if (configs_loaded) return;
    configs_loaded = true;

    cached_design = assets.config("themes/" + active_theme + "/courtroom_design.ini");
    if (!cached_design) cached_design = assets.config("themes/default/courtroom_design.ini");

    cached_fonts = assets.config("themes/" + active_theme + "/courtroom_fonts.ini");
    if (!cached_fonts) cached_fonts = assets.config("themes/default/courtroom_fonts.ini");

    cached_chat_config = assets.config("themes/" + active_theme + "/chat_config.ini");
    if (!cached_chat_config) cached_chat_config = assets.config("themes/default/chat_config.ini");
}

// ---- Character sprites -----------------------------------------------------

std::shared_ptr<ImageAsset> AOAssetLibrary::character_emote(
    const std::string& character, const std::string& emote, const std::string& prefix) {
    std::string base = "characters/" + character + "/";
    auto result = assets.image(base + prefix + emote);
    if (!result && !prefix.empty()) {
        result = assets.image(base + emote);
    }
    return result;
}

std::shared_ptr<ImageAsset> AOAssetLibrary::character_icon(const std::string& character) {
    return assets.image("characters/" + character + "/char_icon");
}

// ---- Background ------------------------------------------------------------

std::shared_ptr<ImageAsset> AOAssetLibrary::background(
    const std::string& name, const std::string& position) {
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

std::shared_ptr<ImageAsset> AOAssetLibrary::desk_overlay(
    const std::string& name, const std::string& position) {
    std::string filename = desk_filename(position);
    auto result = assets.image("background/" + name + "/" + filename);
    if (!result && name != "default") {
        result = assets.image("background/default/" + filename);
    }
    return result;
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
                if (spec.size_pt <= 0) spec.size_pt = 10;
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
    std::string filename = normalized_name + ".ttf";

    // Search: active theme → default → known font-shipping themes → global fonts
    auto result = assets.raw("themes/" + active_theme + "/" + filename);
    if (!result && active_theme != "default")
        result = assets.raw("themes/default/" + filename);
    if (!result) result = assets.raw("themes/AceAttorney DS/" + filename);
    if (!result) result = assets.raw("themes/AceAttorney2x/" + filename);
    if (!result) result = assets.raw("themes/AceAttorney 2x/" + filename);
    if (!result) result = assets.raw("fonts/" + filename);

    if (result) {
        Log::log_print(DEBUG, "AOAssetLibrary: found font '%s'", filename.c_str());
    } else {
        Log::log_print(WARNING, "AOAssetLibrary: font '%s' not found", filename.c_str());
    }

    return result;
}

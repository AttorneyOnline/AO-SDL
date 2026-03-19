#include "ao/game/AOBackground.h"

#include "utils/Log.h"

std::string AOBackground::resolve_bg_filename(const std::string& position) {
    if (position == "def") return "defenseempty";
    if (position == "pro") return "prosecutorempty";
    if (position == "wit") return "witnessempty";
    if (position == "jud") return "judgestand";
    if (position == "hld") return "helperstand";
    if (position == "hlp") return "prohelperstand";
    if (position == "jur") return "jurystand";
    if (position == "sea") return "seancestand";
    return position;
}

std::string AOBackground::resolve_desk_filename(const std::string& position) {
    if (position == "def") return "defensedesk";
    if (position == "pro") return "prosecutiondesk";
    if (position == "wit") return "stand";
    if (position == "jud") return "judgedesk";
    if (position == "hld") return "helperdesk";
    if (position == "hlp") return "prohelperdesk";
    if (position == "jur") return "jurydesk";
    if (position == "sea") return "seancedesk";
    return position + "_overlay";
}

void AOBackground::set(const std::string& background, const std::string& position) {
    if (background != bg_name || position != pos) {
        bg_name = background;
        pos = position;
        dirty = true;
    }
}

void AOBackground::set_position(const std::string& position) {
    if (position != pos) {
        pos = position;
        dirty = true;
    }
}

void AOBackground::reload_if_needed(AssetLibrary& assets) {
    if (!dirty) return;
    dirty = false;

    std::string filename = resolve_bg_filename(pos);
    std::string path = "background/" + bg_name + "/" + filename;

    bg = assets.image(path);
    if (!bg && filename != pos) {
        bg = assets.image("background/" + bg_name + "/" + pos);
    }
    if (!bg && bg_name != "default") {
        bg = assets.image("background/default/" + resolve_bg_filename(pos));
    }

    if (bg) {
        Log::log_print(DEBUG, "Loaded background: %s/%s", bg_name.c_str(), filename.c_str());
    } else {
        Log::log_print(WARNING, "Failed to load background: %s/%s", bg_name.c_str(), filename.c_str());
    }

    std::string desk_filename = resolve_desk_filename(pos);
    desk = assets.image("background/" + bg_name + "/" + desk_filename);
    if (!desk && bg_name != "default") {
        desk = assets.image("background/default/" + desk_filename);
    }
}

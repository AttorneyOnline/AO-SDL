#include "ao/game/AOBackground.h"

#include "utils/Log.h"

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

void AOBackground::reload_if_needed(AOAssetLibrary& ao_assets) {
    if (!dirty) return;
    dirty = false;

    bg = ao_assets.background(bg_name, pos);
    desk = ao_assets.desk_overlay(bg_name, pos);

    if (bg) {
        Log::log_print(DEBUG, "Loaded background: %s/%s", bg_name.c_str(), pos.c_str());
    } else {
        Log::log_print(WARNING, "Failed to load background: %s/%s", bg_name.c_str(), pos.c_str());
    }
}

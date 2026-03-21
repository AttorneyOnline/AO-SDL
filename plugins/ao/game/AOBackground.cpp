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
    // Retry if background was requested but not yet available (HTTP pending)
    if (!dirty && !bg && !bg_name.empty()) {
        bg = ao_assets.background(bg_name, pos);
        if (!bg) {
            // Prefetch via HTTP so it's available next retry (3=BACKGROUND type)
            ao_assets.engine_assets().prefetch_image("background/" + bg_name + "/" + pos, 3);
        } else {
            desk = ao_assets.desk_overlay(bg_name, pos);
        }
        return;
    }

    if (!dirty)
        return;
    dirty = false;

    // Prefetch via HTTP (3=BACKGROUND type)
    ao_assets.engine_assets().prefetch_image("background/" + bg_name + "/" + pos, 3);

    bg = ao_assets.background(bg_name, pos);
    desk = ao_assets.desk_overlay(bg_name, pos);

    if (bg) {
        Log::log_print(DEBUG, "Loaded background: %s/%s", bg_name.c_str(), pos.c_str());
    }
}

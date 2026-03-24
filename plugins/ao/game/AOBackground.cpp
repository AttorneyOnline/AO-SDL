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
    // Retry if background or desk was requested but not yet available (HTTP pending)
    if (!dirty && !bg_name.empty()) {
        if (!bg) {
            bg = ao_assets.background(bg_name, pos);
            if (!bg) {
                ao_assets.prefetch_background(bg_name, pos);
            }
            else {
                desk = ao_assets.desk_overlay(bg_name, pos);
            }
            return;
        }
        if (!desk) {
            desk = ao_assets.desk_overlay(bg_name, pos);
            return;
        }
    }

    if (!dirty)
        return;

    ao_assets.prefetch_background(bg_name, pos);
    ao_assets.engine_assets().prefetch_config("background/" + bg_name + "/design.ini");

    // Try loading the exact background without falling back to default.
    // If the real bg isn't available yet (HTTP pending), show the default
    // as a placeholder but keep dirty=true so we retry on future ticks.
    auto real = ao_assets.background(bg_name, pos, /*no_default=*/true);
    if (real) {
        bg = real;
        desk = ao_assets.desk_overlay(bg_name, pos);
        dirty = false;
        Log::log_print(DEBUG, "Loaded background: %s/%s", bg_name.c_str(), pos.c_str());
    }
    else {
        bg = ao_assets.background(bg_name, pos);
        desk = ao_assets.desk_overlay(bg_name, pos);
    }
}

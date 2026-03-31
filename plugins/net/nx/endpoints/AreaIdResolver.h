/**
 * @file AreaIdResolver.h
 * @brief Shared helper for resolving area_id path parameters.
 *
 * Handles the `_` sentinel (current area) used by multiple area endpoints.
 */
#pragma once

#include "game/AreaState.h"
#include "game/GameRoom.h"
#include "game/ServerSession.h"

#include <string>

/// Resolve an area_id path parameter to an AreaState pointer.
/// Returns nullptr if the area is not found.
///   - "_" resolves to the session's current area.
///   - Any other value is looked up as a hash ID.
inline AreaState* resolve_area(const std::string& area_id, ServerSession* session, GameRoom& room) {
    if (area_id == "_" && session)
        return room.find_area_by_name(session->area);
    return room.find_area(area_id);
}

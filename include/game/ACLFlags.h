/**
 * @file ACLFlags.h
 * @brief Permission flags and role mapping for multi-user authentication.
 *
 * Compatible with akashi's ACL system. Permission flags are bitfields,
 * roles map to a set of permission flags. The two built-in roles are
 * NONE (no permissions) and SUPER (all permissions). Custom roles can
 * be mapped via the acl_permissions_for_role() function.
 */
#pragma once

#include <algorithm>
#include <cctype>
#include <cstdint>
#include <string>

/// Authentication mode.
enum class AuthType {
    SIMPLE,  ///< Single shared mod password → all permissions.
    ADVANCED ///< Per-user database auth with ACL roles.
};

/// Permission flags (bitfield). Matches akashi's Permission enum.
enum class ACLPermission : uint32_t {
    NONE = 0,
    KICK = 1 << 0,
    BAN = 1 << 1,
    BGLOCK = 1 << 2,
    MODIFY_USERS = 1 << 3,
    CM = 1 << 4,
    GLOBAL_TIMER = 1 << 5,
    EVI_MOD = 1 << 6,
    MOTD = 1 << 7,
    ANNOUNCE = 1 << 8,
    MODCHAT = 1 << 9,
    MUTE = 1 << 10,
    UNCM = 1 << 11,
    SAVETEST = 1 << 12,
    FORCE_CHARSELECT = 1 << 13,
    BYPASS_LOCKS = 1 << 14,
    IGNORE_BGLIST = 1 << 15,
    SEND_NOTICE = 1 << 16,
    JUKEBOX = 1 << 17,
    SUPER = 0xffffffff
};

inline ACLPermission operator|(ACLPermission a, ACLPermission b) {
    return static_cast<ACLPermission>(static_cast<uint32_t>(a) | static_cast<uint32_t>(b));
}

inline ACLPermission operator&(ACLPermission a, ACLPermission b) {
    return static_cast<ACLPermission>(static_cast<uint32_t>(a) & static_cast<uint32_t>(b));
}

/// Check if a permission set includes a specific permission.
inline bool has_permission(ACLPermission set, ACLPermission required) {
    if (required == ACLPermission::NONE)
        return true;
    if (set == ACLPermission::SUPER)
        return true;
    return (static_cast<uint32_t>(set) & static_cast<uint32_t>(required)) != 0;
}

/// Map a role ID string to its permission set.
/// Built-in roles: "SUPER" → all permissions, "NONE" → no permissions.
/// Unknown roles are treated as NONE.
inline ACLPermission acl_permissions_for_role(const std::string& role_id) {
    // Uppercase for case-insensitive matching
    std::string upper = role_id;
    std::transform(upper.begin(), upper.end(), upper.begin(), [](unsigned char c) { return std::toupper(c); });

    if (upper == "SUPER")
        return ACLPermission::SUPER;
    if (upper == "NONE" || upper.empty())
        return ACLPermission::NONE;

    // Future: custom roles could be looked up from a roles table/config.
    // For now, treat unknown role names as NONE.
    return ACLPermission::NONE;
}

/// Validate a username: 1-32 chars, alphanumeric + underscore + hyphen.
inline bool is_valid_username(const std::string& name) {
    if (name.empty() || name.size() > 32)
        return false;
    for (char c : name) {
        if (!std::isalnum(static_cast<unsigned char>(c)) && c != '_' && c != '-')
            return false;
    }
    return true;
}

/// Return a human-readable name for a permission flag.
inline const char* permission_name(ACLPermission p) {
    switch (p) {
    case ACLPermission::NONE:
        return "NONE";
    case ACLPermission::KICK:
        return "KICK";
    case ACLPermission::BAN:
        return "BAN";
    case ACLPermission::BGLOCK:
        return "BGLOCK";
    case ACLPermission::MODIFY_USERS:
        return "MODIFY_USERS";
    case ACLPermission::CM:
        return "CM";
    case ACLPermission::GLOBAL_TIMER:
        return "GLOBAL_TIMER";
    case ACLPermission::EVI_MOD:
        return "EVI_MOD";
    case ACLPermission::MOTD:
        return "MOTD";
    case ACLPermission::ANNOUNCE:
        return "ANNOUNCE";
    case ACLPermission::MODCHAT:
        return "MODCHAT";
    case ACLPermission::MUTE:
        return "MUTE";
    case ACLPermission::UNCM:
        return "UNCM";
    case ACLPermission::SAVETEST:
        return "SAVETEST";
    case ACLPermission::FORCE_CHARSELECT:
        return "FORCE_CHARSELECT";
    case ACLPermission::BYPASS_LOCKS:
        return "BYPASS_LOCKS";
    case ACLPermission::IGNORE_BGLIST:
        return "IGNORE_BGLIST";
    case ACLPermission::SEND_NOTICE:
        return "SEND_NOTICE";
    case ACLPermission::JUKEBOX:
        return "JUKEBOX";
    case ACLPermission::SUPER:
        return "SUPER";
    default:
        return "UNKNOWN";
    }
}

#pragma once

#include <string>

/// Generic emote entry exposed to the app/UI layer.
struct EmoteEntry {
    std::string comment;
    std::string pre_anim;
    std::string anim_name;
    int mod = 0;
    int desk_mod = 0;
    std::string sfx_name;
    int sfx_delay = 0;
};

/// Abstract character sheet interface for the app/UI layer.
///
/// Decouples the UI from protocol-specific character data (e.g. AOCharacterSheet).
/// Implementations are provided by protocol plugins.
class ICharacterSheet {
  public:
    virtual ~ICharacterSheet() = default;

    virtual const std::string& name() const = 0;
    virtual const std::string& showname() const = 0;
    virtual const std::string& side() const = 0;

    virtual int emote_count() const = 0;
    virtual EmoteEntry emote(int index) const = 0;
};

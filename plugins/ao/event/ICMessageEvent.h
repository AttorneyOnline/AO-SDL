/**
 * @file ICMessageEvent.h
 * @brief Event carrying an in-character (IC) message for courtroom display.
 * @ingroup events
 */
#pragma once

#include "event/Event.h"

#include <string>

/**
 * @brief Controls how the character animation sequence plays.
 *
 * Mirrors the AO2 EMOTE_MOD field in the MS packet.
 */
enum class EmoteMod {
    IDLE = 0,         ///< Skip pre-animation, show idle immediately.
    PREANIM = 1,      ///< Play pre-animation, then talking animation.
    ZOOM = 5,         ///< Zoom effect + talking (no pre-animation).
    PREANIM_ZOOM = 6, ///< Pre-animation + zoom + talking.
};

/**
 * @brief Controls desk overlay visibility during this message.
 */
enum class DeskMod {
    CHAT = -1,         ///< Default: desk shown for def/pro/wit, hidden for hld/hlp/jud.
    HIDE = 0,          ///< Hide desk.
    SHOW = 1,          ///< Show desk.
    EMOTE_ONLY = 2,    ///< Show desk only during emote.
    PRE_ONLY = 3,      ///< Show desk only during pre-animation.
    EMOTE_ONLY_EX = 4, ///< Extended emote-only mode.
    PRE_ONLY_EX = 5,   ///< Extended pre-only mode.
};

/**
 * @brief Signals an in-character message was received from the server.
 * @ingroup events
 *
 * Published by the protocol plugin when the server sends an IC message
 * (the AO2 MS packet). The courtroom presenter consumes this to drive
 * character animations, background changes, and desk visibility.
 *
 * Currently only carries the fields needed for character emote display.
 * Text, sound, and effects fields will be added later.
 */
class ICMessageEvent : public Event {
  public:
    ICMessageEvent(std::string character, std::string emote, std::string pre_emote, std::string message,
                   std::string showname, std::string side, EmoteMod emote_mod, DeskMod desk_mod, bool flip, int char_id,
                   int text_color, int objection_mod, bool screenshake, bool realization, bool additive,
                   std::string frame_screenshake, std::string sfx_name, int sfx_delay, bool sfx_looping,
                   std::string frame_sfx, bool immediate = false);

    std::string to_string() const override;

    const std::string& get_character() const {
        return character;
    }
    const std::string& get_emote() const {
        return emote;
    }
    const std::string& get_pre_emote() const {
        return pre_emote;
    }
    const std::string& get_message() const {
        return message;
    }
    const std::string& get_showname() const {
        return showname;
    }
    const std::string& get_side() const {
        return side;
    }
    EmoteMod get_emote_mod() const {
        return emote_mod;
    }
    DeskMod get_desk_mod() const {
        return desk_mod;
    }
    bool get_flip() const {
        return flip;
    }
    int get_char_id() const {
        return char_id;
    }
    int get_text_color() const {
        return text_color;
    }
    int get_objection_mod() const {
        return objection_mod;
    }
    bool get_screenshake() const {
        return screenshake;
    }
    bool get_realization() const {
        return realization;
    }
    bool get_additive() const {
        return additive;
    }
    const std::string& get_frame_screenshake() const {
        return frame_screenshake;
    }
    const std::string& get_sfx_name() const {
        return sfx_name;
    }
    int get_sfx_delay() const {
        return sfx_delay;
    }
    bool get_sfx_looping() const {
        return sfx_looping;
    }
    const std::string& get_frame_sfx() const {
        return frame_sfx;
    }
    bool get_immediate() const {
        return immediate;
    }

  private:
    std::string character;
    std::string emote;
    std::string pre_emote;
    std::string message;
    std::string showname;
    std::string side;
    EmoteMod emote_mod;
    DeskMod desk_mod;
    bool flip;
    int char_id;
    int text_color;
    int objection_mod;
    bool screenshake;
    bool realization;
    bool additive;
    std::string frame_screenshake;
    std::string sfx_name;
    int sfx_delay;
    bool sfx_looping;
    std::string frame_sfx;
    bool immediate;
};

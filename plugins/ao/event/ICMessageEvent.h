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
    ICMessageEvent(std::string character, std::string emote, std::string pre_emote,
                   std::string side, EmoteMod emote_mod, DeskMod desk_mod,
                   bool flip, int char_id);

    std::string to_string() const override;

    /** @brief Character folder name (e.g. "Phoenix"). */
    const std::string& get_character() const { return character; }

    /** @brief Base emote name (e.g. "normal", "pointing"). Used to build (a)/(b) paths. */
    const std::string& get_emote() const { return emote; }

    /** @brief Pre-animation emote filename (from char.ini, e.g. "objecting"). */
    const std::string& get_pre_emote() const { return pre_emote; }

    /** @brief Courtroom position (e.g. "def", "pro", "wit"). */
    const std::string& get_side() const { return side; }

    /** @brief Animation mode controlling the pre-anim → talking → idle sequence. */
    EmoteMod get_emote_mod() const { return emote_mod; }

    /** @brief Desk overlay visibility mode. */
    DeskMod get_desk_mod() const { return desk_mod; }

    /** @brief Whether the character sprite should be flipped horizontally. */
    bool get_flip() const { return flip; }

    /** @brief Character ID in the server's character list. */
    int get_char_id() const { return char_id; }

  private:
    std::string character;
    std::string emote;
    std::string pre_emote;
    std::string side;
    EmoteMod emote_mod;
    DeskMod desk_mod;
    bool flip;
    int char_id;
};

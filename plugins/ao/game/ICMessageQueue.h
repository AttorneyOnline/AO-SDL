#pragma once

#include "ao/event/ICMessageEvent.h"

#include <deque>
#include <functional>
#include <optional>
#include <string>

/// Captured IC message data, independent of the event system lifetime.
struct ICMessage {
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
    bool screenshake;
    bool realization;
    bool additive;
    int objection_mod;

    static ICMessage from_event(const ICMessageEvent& ev) {
        return {ev.get_character(), ev.get_emote(),       ev.get_pre_emote(),   ev.get_message(),
                ev.get_showname(),  ev.get_side(),        ev.get_emote_mod(),   ev.get_desk_mod(),
                ev.get_flip(),      ev.get_char_id(),     ev.get_text_color(),  ev.get_screenshake(),
                ev.get_realization(), ev.get_additive(),  ev.get_objection_mod()};
    }

    bool is_objection() const { return objection_mod >= 1 && objection_mod <= 4; }
};

/// Queues IC messages so they play sequentially without stepping on each other.
///
/// When a message is playing (text scrolling + emote animating), new messages
/// wait in the queue. When the current message finishes and a linger period
/// elapses, the next message is dequeued.
///
/// Objections (objection_mod 1-4) clear the queue and play immediately.
class ICMessageQueue {
  public:
    /// Set a callback invoked when a message is enqueued but won't play
    /// immediately. Use this to prefetch assets (emotes, backgrounds) so
    /// they're cache-warm by the time the message plays.
    void set_prefetch(std::function<void(const ICMessage&)> fn) { prefetch_ = std::move(fn); }

    /// Add a message. If nothing is playing, it becomes immediately available
    /// via next(). Objections clear the queue and interrupt.
    void enqueue(ICMessage msg);

    /// Call each tick. Advances the linger timer when the current message
    /// is done. Returns true if a new message is ready to play.
    bool tick(int delta_ms, bool current_message_done);

    /// Get the next message to play (and mark it as "playing").
    /// Returns nullopt if nothing is ready.
    std::optional<ICMessage> next();

    /// Whether a message is currently being presented.
    bool is_playing() const { return playing_; }

    /// Number of messages waiting in the queue.
    size_t pending() const { return queue_.size(); }

  private:
    std::deque<ICMessage> queue_;
    std::function<void(const ICMessage&)> prefetch_;
    bool playing_ = false;
    bool ready_ = false;

    // Linger: how long to wait after text finishes before auto-advancing
    int linger_ms_ = 0;
    static constexpr int LINGER_DURATION_MS = 300;
};

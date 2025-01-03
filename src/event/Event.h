#ifndef EVENT_H
#define EVENT_H

#include <string>

// todo: investigate a more generic/extensible way to do this
// it might prove somewhat cumbersome to add more target types here in the future. or not. this might be fine, ultimately. regardless, it merits investigation
enum EventTarget {
    NETWORK,
    UI,
    CHAT,
    LOG
};

class Event {
  public:
    Event(EventTarget target);

    virtual std::string to_string() const;
    EventTarget get_target() const;

  private:
    EventTarget target;
};

#endif
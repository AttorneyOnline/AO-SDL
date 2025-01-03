#ifndef EVENT_H
#define EVENT_H

#include <string>

class Event {
  public:
    Event();

    virtual std::string to_string();

  private:
};

#endif
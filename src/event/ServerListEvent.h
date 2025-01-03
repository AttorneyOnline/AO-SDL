#ifndef SERVERLISTEVENT_H
#define SERVERLISTEVENT_H

#include "Event.h"

#include "game/ServerList.h"

class ServerListEvent : public Event {
  public:
    ServerListEvent(ServerList server_list, EventTarget target);

    std::string to_string() const override;
    ServerList get_server_list();

  private:
    ServerList server_list;
};

#endif
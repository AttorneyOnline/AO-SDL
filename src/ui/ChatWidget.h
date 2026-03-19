#pragma once

#include <string>

// Shared chat UI component — drop into any Screen that needs a chat panel.
class ChatWidget {
  public:
    void handle_events();
    void render();

  private:
    std::string buffer;
    char name[32] = "";
    char message[1024] = "";
};

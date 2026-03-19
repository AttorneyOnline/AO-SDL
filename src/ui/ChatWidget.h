#pragma once

#include <string>

// Shared chat UI component — drop into any Screen that needs a chat panel.
class ChatWidget {
  public:
    void handle_events();
    void render();

  private:
    std::string m_buffer;
    char m_name[32] = "";
    char m_message[1024] = "";
};

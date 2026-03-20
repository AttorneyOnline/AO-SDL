#pragma once

#include "ui/IWidget.h"

#include <string>

class ChatWidget : public IWidget {
  public:
    void handle_events() override;
    void render() override;

  private:
    std::string m_buffer;
    char m_name[32] = "";
    char m_message[1024] = "";
};

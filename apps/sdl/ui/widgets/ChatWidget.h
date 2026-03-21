#pragma once

#include "ui/IWidget.h"

#include <string>

class ChatWidget : public IWidget {
  public:
    void handle_events() override;
    void render() override;

    /// Consume the debug toggle flag (set when user types /debug).
    bool consume_debug_toggle() {
        bool v = debug_toggled_;
        debug_toggled_ = false;
        return v;
    }

  private:
    /// Shared OOC log buffer that persists across screen transitions.
    static std::string& shared_buffer();

    char m_name[32] = "";
    char m_message[1024] = "";
    bool debug_toggled_ = false;
};

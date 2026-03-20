#pragma once

#include "ui/IWidget.h"

#include <string>

class DisconnectModalWidget : public IWidget {
  public:
    void handle_events() override;
    void render() override;

    bool should_return_to_server_list() const {
        return return_to_server_list_;
    }
    void clear_flag() {
        return_to_server_list_ = false;
    }

  private:
    bool show_modal_ = false;
    bool return_to_server_list_ = false;
    std::string reason_;
};

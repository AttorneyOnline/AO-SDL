#pragma once

#include "ui/IWidget.h"

#include <string>

class ICLogWidget : public IWidget {
  public:
    void handle_events() override;
    void render() override;

  private:
    std::string buffer_;
};

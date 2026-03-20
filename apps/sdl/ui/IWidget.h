#pragma once

class IWidget {
  public:
    virtual ~IWidget() = default;
    virtual void handle_events() = 0;
    virtual void render() = 0;
};

#pragma once

#include "renderer/IGPUBackend.h"
#include "ui/UIManager.h"

#include <functional>
#include <memory>

class QQuickView;

class QtGameWindow {
  public:
    QtGameWindow(UIManager &ui_manager, std::unique_ptr<IGPUBackend> backend);

  private:
    QQuickView *window;
    UIManager &ui_manager;
    std::unique_ptr<IGPUBackend> gpu;
    std::function<void()> frame_callback_;
};

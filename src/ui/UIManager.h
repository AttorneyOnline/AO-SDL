#ifndef UIMANAGER_H
#define UIMANAGER_H

#include "render/RenderManager.h"
#include "event/EventChannel.h"
#include "event/UIEvent.h"

enum UIView {
    SERVER_LIST,
    CHAR_SELECT,
    COURTROOM
};

class UIManager {
  public:
    UIManager();

    UIView get_current_view();
    void render_current_view(RenderManager& render);
    void handle_events();

  private:
    UIView current_view;

    // todo: bad, do this right
    std::string chat_buffer;
    char chat_name[32] = "";
    char chat_message[1024] = "";
};

#endif
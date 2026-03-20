#include "ui/screens/CourtroomScreen.h"

void CourtroomScreen::enter(ScreenController&) {
}

void CourtroomScreen::exit() {
}

void CourtroomScreen::handle_events() {
    chat.handle_events();
}

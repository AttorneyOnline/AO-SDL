#include "ao/ui/screens/CourtroomScreen.h"

CourtroomScreen::CourtroomScreen(std::string character_name, int char_id)
    : character_name_(std::move(character_name)), char_id_(char_id) {
}

void CourtroomScreen::enter(ScreenController&) {
}

void CourtroomScreen::exit() {
}

void CourtroomScreen::handle_events() {
}

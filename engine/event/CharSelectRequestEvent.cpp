#include "event/CharSelectRequestEvent.h"

CharSelectRequestEvent::CharSelectRequestEvent(int char_id) : char_id(char_id) {
}

int CharSelectRequestEvent::get_char_id() const {
    return char_id;
}

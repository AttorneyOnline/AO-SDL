#include "CharSelectRequestEvent.h"

CharSelectRequestEvent::CharSelectRequestEvent(int char_id) : m_char_id(char_id) {
}

int CharSelectRequestEvent::get_char_id() const {
    return m_char_id;
}

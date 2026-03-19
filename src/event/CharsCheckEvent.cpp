#include "CharsCheckEvent.h"

CharsCheckEvent::CharsCheckEvent(std::vector<bool> taken) : m_taken(std::move(taken)) {
}

const std::vector<bool>& CharsCheckEvent::get_taken() const {
    return m_taken;
}

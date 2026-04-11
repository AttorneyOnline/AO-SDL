#include "EvidenceController.h"

#include "event/EventManager.h"
#include "event/EvidenceListEvent.h"

EvidenceController::EvidenceController(QObject* parent) : IQtScreenController(parent) {
}

void EvidenceController::drain() {
    auto& ch = EventManager::instance().get_channel<EvidenceListEvent>();
    while (auto ev = ch.get_event())
        m_evidence.reset(ev->items());
}

void EvidenceController::selectEvidence(int index) {
    if (index == m_selectedIndex)
        return;
    m_selectedIndex = index;
    emit selectedIndexChanged();
}

void EvidenceController::reset() {
    m_evidence.clear();
    if (m_selectedIndex != -1) {
        m_selectedIndex = -1;
        emit selectedIndexChanged();
    }
}

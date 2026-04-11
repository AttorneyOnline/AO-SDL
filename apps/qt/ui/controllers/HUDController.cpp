#include "HUDController.h"

#include "event/EventManager.h"
#include "event/HealthBarEvent.h"
#include "event/TimerEvent.h"

HUDController::HUDController(QObject* parent) : IQtScreenController(parent) {
}

void HUDController::drain() {
    auto& hpCh = EventManager::instance().get_channel<HealthBarEvent>();
    while (auto ev = hpCh.get_event()) {
        if (ev->side() == 1 && ev->value() != m_defHp) {
            m_defHp = ev->value();
            emit defHpChanged();
        }
        else if (ev->side() == 2 && ev->value() != m_proHp) {
            m_proHp = ev->value();
            emit proHpChanged();
        }
    }

    auto& timerCh = EventManager::instance().get_channel<TimerEvent>();
    while (auto ev = timerCh.get_event()) {
        // Only consume timer_id 0 (main courtroom timer).
        // action: 0=start/sync, 1=pause, 2=show, 3=hide.
        if (ev->timer_id() != 0)
            continue;
        int secs = (ev->action() == 0 || ev->action() == 2) ? static_cast<int>(ev->time_ms() / 1000) : 0;
        if (secs != m_timerSeconds) {
            m_timerSeconds = secs;
            emit timerSecondsChanged();
        }
    }
}

void HUDController::reset() {
    if (m_defHp != 0) {
        m_defHp = 0;
        emit defHpChanged();
    }
    if (m_proHp != 0) {
        m_proHp = 0;
        emit proHpChanged();
    }
    if (m_timerSeconds != 0) {
        m_timerSeconds = 0;
        emit timerSecondsChanged();
    }
}

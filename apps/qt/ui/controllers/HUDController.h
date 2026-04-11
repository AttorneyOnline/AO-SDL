#pragma once

#include "IQtScreenController.h"

#include <QObject>
#include <QString>

/**
 * @brief Qt controller for the in-game HUD: health bars and server timer.
 *
 * drain() consumes HealthBarEvent and TimerEvent.
 * Emits interjectionTriggered(word) when an IC interjection packet fires;
 * CourtroomScreen connects this to InterjectionOverlay::show().
 */
class HUDController : public IQtScreenController {
    Q_OBJECT
    Q_PROPERTY(int defHp READ defHp NOTIFY defHpChanged)
    Q_PROPERTY(int proHp READ proHp NOTIFY proHpChanged)
    Q_PROPERTY(int timerSeconds READ timerSeconds NOTIFY timerSecondsChanged)

  public:
    explicit HUDController(QObject* parent = nullptr);

    void drain() override;

    int defHp() const {
        return m_defHp;
    }
    int proHp() const {
        return m_proHp;
    }
    int timerSeconds() const {
        return m_timerSeconds;
    }

    void reset();

  signals:
    void defHpChanged();
    void proHpChanged();
    void timerSecondsChanged();
    void interjectionTriggered(const QString& word);

  private:
    int m_defHp = 0;
    int m_proHp = 0;
    int m_timerSeconds = 0;
};

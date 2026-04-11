#pragma once

#include "IQtScreenController.h"

#include <QObject>
#include <QString>

/**
 * @brief Global Qt controller for audio state.
 *
 * Registered as a permanent drain channel — always active regardless of
 * which screen is visible.
 *
 * drain() consumes NowPlayingEvent and exposes nowPlaying to QML.
 * Actual audio playback (AudioThread + IAudioDevice) is wired in the
 * audio implementation phase.
 */
class AudioController : public IQtScreenController {
    Q_OBJECT
    Q_PROPERTY(QString nowPlaying READ nowPlaying NOTIFY nowPlayingChanged)

  public:
    explicit AudioController(QObject* parent = nullptr);

    void drain() override;

    QString nowPlaying() const {
        return m_nowPlaying;
    }

    void reset();

  signals:
    void nowPlayingChanged();

  private:
    QString m_nowPlaying;
};

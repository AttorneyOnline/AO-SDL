#include "AudioController.h"

#include "event/EventManager.h"
#include "event/NowPlayingEvent.h"

AudioController::AudioController(QObject* parent) : IQtScreenController(parent) {
}

void AudioController::drain() {
    auto& ch = EventManager::instance().get_channel<NowPlayingEvent>();
    while (auto ev = ch.get_event()) {
        QString np = QString::fromStdString(ev->track());
        if (np != m_nowPlaying) {
            m_nowPlaying = np;
            emit nowPlayingChanged();
        }
    }
}

void AudioController::reset() {
    if (!m_nowPlaying.isEmpty()) {
        m_nowPlaying.clear();
        emit nowPlayingChanged();
    }
}

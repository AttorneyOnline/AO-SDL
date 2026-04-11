#include "MusicAreaController.h"

#include "event/AreaUpdateEvent.h"
#include "event/EventManager.h"
#include "event/MusicListEvent.h"
#include "event/OutgoingMusicEvent.h"

#include <cstdlib>

MusicAreaController::MusicAreaController(QObject* parent) : IQtScreenController(parent) {
}

void MusicAreaController::drain() {
    drainMusicList();
    drainAreaUpdates();
}

void MusicAreaController::playTrack(const QString& name, const QString& showname) {
    EventManager::instance().get_channel<OutgoingMusicEvent>().publish(
        OutgoingMusicEvent(name.toStdString(), showname.toStdString()));
}

void MusicAreaController::changeArea(const QString& name) {
    EventManager::instance().get_channel<OutgoingMusicEvent>().publish(OutgoingMusicEvent(name.toStdString()));
}

void MusicAreaController::reset() {
    m_musicArea.clear();
    m_areas.clear();
    m_tracks.clear();
}

void MusicAreaController::drainMusicList() {
    auto& ch = EventManager::instance().get_channel<MusicListEvent>();
    bool dirty = false;
    while (auto ev = ch.get_event()) {
        dirty = true;
        if (ev->partial()) {
            if (!ev->areas().empty()) {
                m_areas.clear();
                m_areas.resize(ev->areas().size());
                for (size_t i = 0; i < ev->areas().size(); i++)
                    m_areas[i].name = ev->areas()[i];
            }
            if (!ev->tracks().empty())
                m_tracks = ev->tracks();
        }
        else {
            m_areas.clear();
            m_areas.resize(ev->areas().size());
            for (size_t i = 0; i < ev->areas().size(); i++)
                m_areas[i].name = ev->areas()[i];
            m_tracks = ev->tracks();
        }
    }
    if (dirty)
        rebuildModel();
}

void MusicAreaController::drainAreaUpdates() {
    auto& ch = EventManager::instance().get_channel<AreaUpdateEvent>();
    bool dirty = false;
    while (auto ev = ch.get_event()) {
        dirty = true;
        const auto& vals = ev->values();
        size_t count = std::min(vals.size(), m_areas.size());
        for (size_t i = 0; i < count; i++) {
            switch (ev->type()) {
            case AreaUpdateEvent::PLAYERS:
                m_areas[i].playerCount = std::atoi(vals[i].c_str());
                break;
            case AreaUpdateEvent::STATUS:
                m_areas[i].status = vals[i];
                break;
            case AreaUpdateEvent::CM:
                m_areas[i].cm = vals[i];
                break;
            case AreaUpdateEvent::LOCK:
                m_areas[i].lock = vals[i];
                break;
            }
        }
    }
    if (dirty)
        rebuildModel();
}

void MusicAreaController::rebuildModel() {
    std::vector<MusicAreaModel::Entry> entries;
    entries.reserve(m_areas.size() + m_tracks.size());

    for (const auto& a : m_areas) {
        MusicAreaModel::Entry e;
        e.name = QString::fromStdString(a.name);
        e.playerCount = a.playerCount;
        e.isArea = true;
        e.status = QString::fromStdString(a.status);
        e.cm = QString::fromStdString(a.cm);
        e.lock = QString::fromStdString(a.lock);
        entries.push_back(std::move(e));
    }
    for (const auto& t : m_tracks) {
        MusicAreaModel::Entry e;
        e.name = QString::fromStdString(t);
        e.isArea = false;
        entries.push_back(std::move(e));
    }

    m_musicArea.reset(entries);
}

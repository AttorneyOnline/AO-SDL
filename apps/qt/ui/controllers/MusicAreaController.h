#pragma once

#include "IQtScreenController.h"
#include "ui/models/MusicAreaModel.h"

#include <QObject>
#include <QString>

#include <algorithm>
#include <string>
#include <vector>

/**
 * @brief Qt controller for the combined music track and area list.
 *
 * drain() consumes MusicListEvent and AreaUpdateEvent, incrementally
 * maintaining area metadata and rebuilding MusicAreaModel when either changes.
 *
 * playTrack() and changeArea() both publish OutgoingMusicEvent (MC packet).
 */
class MusicAreaController : public IQtScreenController {
    Q_OBJECT
    Q_PROPERTY(MusicAreaModel* musicAreaModel READ musicAreaModel CONSTANT)

  public:
    explicit MusicAreaController(QObject* parent = nullptr);

    void drain() override;

    MusicAreaModel* musicAreaModel() {
        return &m_musicArea;
    }

    Q_INVOKABLE void playTrack(const QString& name, const QString& showname = {});
    Q_INVOKABLE void changeArea(const QString& name);

    void reset();

  private:
    struct AreaState {
        std::string name;
        int playerCount = -1;
        std::string status = "Unknown";
        std::string cm = "Unknown";
        std::string lock = "Unknown";
    };

    void drainMusicList();
    void drainAreaUpdates();
    void rebuildModel();

    MusicAreaModel m_musicArea;
    std::vector<AreaState> m_areas;
    std::vector<std::string> m_tracks;
};

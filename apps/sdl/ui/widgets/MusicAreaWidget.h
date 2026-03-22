#pragma once

#include "ui/IWidget.h"

#include <string>
#include <vector>

struct ICMessageState;

class MusicAreaWidget : public IWidget {
  public:
    explicit MusicAreaWidget(ICMessageState* state) : state_(state) {
    }

    void handle_events() override;
    void render() override;

    /// Current track name (set by incoming MusicChangeEvent).
    const std::string& now_playing() const {
        return now_playing_;
    }

  private:
    ICMessageState* state_;

    int music_vol_ = 50;
    int sfx_vol_ = 50;
    int blip_vol_ = 50;

    std::vector<std::string> areas_;
    std::vector<std::string> tracks_;
    std::vector<std::string> tracks_lower_; // pre-lowercased for filtering
    std::string now_playing_;
    char search_buf_[128] = "";
    int selected_area_ = -1;

    // Per-area metadata from ARUP packets (parallel to areas_)
    std::vector<int> area_players_;
    std::vector<std::string> area_status_;
    std::vector<std::string> area_cm_;
    std::vector<std::string> area_lock_;
};

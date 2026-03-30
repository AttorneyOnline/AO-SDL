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

  private:
    ICMessageState* state_;

    int music_vol_ = 50;
    int sfx_vol_ = 50;
    int blip_vol_ = 50;

    void rebuild_track_caches();

    std::vector<std::string> tracks_trimmed_;
    std::vector<std::string> tracks_lower_; // pre-lowercased for filtering
    char search_buf_[128] = "";
    int selected_area_ = -1;
};

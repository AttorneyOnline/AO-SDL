#pragma once

#include "ui/IWidget.h"

#include <array>
#include <string>
#include <unordered_map>
#include <vector>

struct DebugStats {
    float frame_time_ms = 0;
    float fps = 0;
    float game_tick_ms = 0;
    float tick_rate_hz = 0;

    struct TickSection {
        const char* name;
        float us;
    };
    std::vector<TickSection> tick_sections;

    const char* gpu_backend = "Unknown";
    int draw_calls = 0;

    struct CacheEntry {
        std::string path;
        std::string format;
        size_t bytes;
        long use_count;
    };
    std::vector<CacheEntry> cache_entries;
    size_t cache_used_bytes = 0;
    size_t cache_max_bytes = 0;

    std::string server_software;
    std::string server_version;
    int current_players = 0;
    int max_players = 0;
    int conn_state = 0;
};

class DebugOverlayWidget : public IWidget {
  public:
    void handle_events() override;
    void render() override;

    DebugStats& stats() { return stats_; }

  private:
    void draw_pie(const std::vector<std::pair<const char*, float>>& slices);

    DebugStats stats_;

    // Rolling average for tick sections
    static constexpr int AVG_WINDOW = 30;
    struct RingBuffer {
        std::array<float, AVG_WINDOW> samples = {};
        int write_pos = 0;
        bool filled = false;

        void push(float v) {
            samples[write_pos] = v;
            write_pos = (write_pos + 1) % AVG_WINDOW;
            if (write_pos == 0) filled = true;
        }
        float average() const {
            int count = filled ? AVG_WINDOW : write_pos;
            if (count == 0) return 0;
            float sum = 0;
            for (int i = 0; i < count; i++) sum += samples[i];
            return sum / count;
        }
    };
    std::unordered_map<const char*, RingBuffer> tick_avg_;
    int frame_count_ = 0;
};

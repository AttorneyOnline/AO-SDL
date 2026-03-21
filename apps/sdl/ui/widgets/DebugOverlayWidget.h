#pragma once

#include "ui/IWidget.h"
#include "utils/Log.h"

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
    void draw_history();

    DebugStats stats_;

    // Rolling average for tick sections (pie chart)
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

    // History for line graph (~30s at 60Hz = 1800 samples)
    static constexpr int HISTORY_SIZE = 1800;
    struct HistoryRing {
        std::array<float, HISTORY_SIZE> data = {};
        int write_pos = 0;
        int count = 0;

        void push(float v) {
            data[write_pos] = v;
            write_pos = (write_pos + 1) % HISTORY_SIZE;
            if (count < HISTORY_SIZE) count++;
        }
        float at(int i) const {
            // i=0 is oldest, i=count-1 is newest
            int idx = (write_pos - count + i + HISTORY_SIZE) % HISTORY_SIZE;
            return data[idx];
        }
    };
    std::unordered_map<const char*, HistoryRing> tick_history_;

    // --- Log viewer ---
    void draw_log();

    bool log_filter_[LogLevel::COUNT] = {false, false, true, true, true, true, true};
    char log_search_[128] = "";
    bool log_auto_scroll_ = true;
};

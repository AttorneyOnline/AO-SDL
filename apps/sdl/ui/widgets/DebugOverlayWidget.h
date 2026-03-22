#pragma once

#include "asset/ImageAsset.h"
#include "ui/IWidget.h"
#include "ui/LogBuffer.h"
#include "utils/Log.h"

#include <array>
#include <deque>
#include <memory>
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
    bool uv_flipped = false; // true for GL (V=0 at bottom)

    struct CacheEntry {
        std::string path;
        std::string format;
        size_t bytes;
        long use_count;
        int width = 0, height = 0;
        int frame_count = 0;
        std::shared_ptr<ImageAsset> image; // for texture preview
        uintptr_t texture_id = 0;          // GPU texture handle for ImGui::Image
    };
    std::vector<CacheEntry> cache_entries;
    size_t cache_used_bytes = 0;
    size_t cache_max_bytes = 0;

    std::string server_software;
    std::string server_version;
    int current_players = 0;
    int max_players = 0;
    int conn_state = 0;

    // HTTP asset streaming stats
    int http_pending = 0;
    int http_cached = 0;
    int http_failed = 0;
    int http_pool_pending = 0;
    size_t http_cached_bytes = 0;

    struct HttpCacheEntry {
        std::string path;
        size_t bytes;
    };
    std::vector<HttpCacheEntry> http_cache_entries;

    // Event stats
    struct EventStat {
        std::string name;
        uint64_t count;
    };
    std::vector<EventStat> event_stats;

    // Audio stats
    struct AudioChannelInfo {
        int id;
        bool active;
        bool is_stream;
        bool stream_ready;
        bool stream_finished;
        bool looping;
        int64_t loop_start;
        int64_t loop_end;
        float volume;
        size_t ring_available;
    };
    std::vector<AudioChannelInfo> audio_channels;
    std::string now_playing;
};

class DebugOverlayWidget : public IWidget {
  public:
    void handle_events() override;
    void render() override;

    DebugStats& stats() {
        return stats_;
    }

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
            if (write_pos == 0)
                filled = true;
        }
        float average() const {
            int count = filled ? AVG_WINDOW : write_pos;
            if (count == 0)
                return 0;
            float sum = 0;
            for (int i = 0; i < count; i++)
                sum += samples[i];
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
            if (count < HISTORY_SIZE)
                count++;
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
    std::deque<LogBuffer::Entry> log_local_;
    size_t log_gen_ = 0;

    // Asset cache viewer
    int selected_cache_entry_ = -1;
    std::string selected_cache_path_;
    enum CacheSortMode { SORT_NAME, SORT_LRU, SORT_REFS, SORT_SIZE };
    CacheSortMode cache_sort_ = SORT_NAME;
    char cache_search_[128] = "";
};

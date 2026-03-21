#include "ui/widgets/DebugOverlayWidget.h"

#include "ui/DebugContext.h"
#include "ui/LogBuffer.h"

#include <imgui.h>

#include <algorithm>
#include <cmath>
#include <cstdio>

void DebugOverlayWidget::handle_events() {
}

static const char* format_bytes(size_t bytes, char* buf, size_t buf_size) {
    if (bytes >= 1024 * 1024)
        std::snprintf(buf, buf_size, "%.1f MB", bytes / (1024.0 * 1024.0));
    else if (bytes >= 1024)
        std::snprintf(buf, buf_size, "%.1f KB", bytes / 1024.0);
    else
        std::snprintf(buf, buf_size, "%zu B", bytes);
    return buf;
}

static const char* conn_state_str(int s) {
    switch (s) {
    case 0: return "Disconnected";
    case 1: return "Connected";
    case 2: return "Joined";
    default: return "Unknown";
    }
}

static constexpr ImU32 SLICE_COLORS[] = {
    IM_COL32(100, 200, 100, 255),
    IM_COL32(100, 150, 255, 255),
    IM_COL32(255, 180, 80,  255),
    IM_COL32(200, 100, 200, 255),
    IM_COL32(255, 255, 100, 255),
    IM_COL32(100, 220, 220, 255),
    IM_COL32(255, 100, 100, 255),
};
static constexpr int NUM_COLORS = sizeof(SLICE_COLORS) / sizeof(SLICE_COLORS[0]);

void DebugOverlayWidget::draw_pie(const std::vector<std::pair<const char*, float>>& slices) {
    float total = 0;
    for (const auto& [_, v] : slices) total += v;
    if (total <= 0) return;

    constexpr float PI = 3.14159265f;
    constexpr float PI2 = 2.0f * PI;
    float radius = 28.0f;
    int segments = 48;

    ImVec2 cursor = ImGui::GetCursorScreenPos();
    ImVec2 center = {cursor.x + radius + 2, cursor.y + radius + 2};
    ImDrawList* dl = ImGui::GetWindowDrawList();

    float angle = -PI / 2.0f; // start at top
    for (int i = 0; i < (int)slices.size(); i++) {
        float frac = slices[i].second / total;
        float next_angle = angle + frac * PI2;
        int arc_segments = std::max(3, (int)(frac * segments));

        // Build a triangle fan: center, then arc points
        dl->PathClear();
        dl->PathLineTo(center);
        for (int s = 0; s <= arc_segments; s++) {
            float a = angle + (next_angle - angle) * ((float)s / arc_segments);
            dl->PathLineTo({center.x + std::cos(a) * radius, center.y + std::sin(a) * radius});
        }
        dl->PathFillConvex(SLICE_COLORS[i % NUM_COLORS]);

        angle = next_angle;
    }

    // Outline
    dl->AddCircle(center, radius, IM_COL32(80, 80, 80, 255), segments, 1.5f);

    // Legend to the right
    float legend_x = cursor.x + radius * 2 + 12;
    float legend_y = cursor.y;

    for (int i = 0; i < (int)slices.size(); i++) {
        float pct = slices[i].second / total * 100.0f;

        ImU32 col = SLICE_COLORS[i % NUM_COLORS];

        // Color swatch
        dl->AddRectFilled({legend_x, legend_y + 2}, {legend_x + 8, legend_y + 10}, col);

        char label[128];
        std::snprintf(label, sizeof(label), " %s: %.0f us (%.0f%%)", slices[i].first, slices[i].second, pct);
        dl->AddText({legend_x + 10, legend_y}, IM_COL32(220, 220, 220, 255), label);
        legend_y += ImGui::GetTextLineHeightWithSpacing();
    }

    float h = std::max(radius * 2 + 4, legend_y - cursor.y + 4);
    ImGui::Dummy(ImVec2(ImGui::GetContentRegionAvail().x, h));
}

void DebugOverlayWidget::render() {

    auto& s = stats_;
    char buf[64], buf2[64];

    // Push samples into averaging buffers
    for (const auto& sec : s.tick_sections) {
        tick_avg_[sec.name].push(sec.us);
        tick_history_[sec.name].push(sec.us);
    }
    frame_count_++;

    // --- Performance ---
    ImGui::SeparatorText("Performance");
    ImGui::Text("FPS: %.0f  |  Frame: %.2f ms", s.fps, s.frame_time_ms);
    ImGui::Text("Game tick: %.3f ms @ %.0f Hz", s.game_tick_ms, s.tick_rate_hz);
    ImGui::Text("Draw calls: %d", s.draw_calls);

    // --- Game tick breakdown ---
    if (!s.tick_sections.empty() && frame_count_ >= 5) {
        ImGui::SeparatorText("Tick Breakdown");
        if (ImGui::BeginTabBar("##tick_tabs")) {
            if (ImGui::BeginTabItem("Pie")) {
                std::vector<std::pair<const char*, float>> slices;
                for (const auto& sec : s.tick_sections)
                    slices.push_back({sec.name, tick_avg_[sec.name].average()});
                draw_pie(slices);
                ImGui::EndTabItem();
            }
            if (ImGui::BeginTabItem("History")) {
                draw_history();
                ImGui::EndTabItem();
            }
            ImGui::EndTabBar();
        }
    }

    // --- Renderer ---
    ImGui::SeparatorText("Renderer");
    ImGui::Text("Backend: %s", s.gpu_backend);

    auto& ctx = DebugContext::instance();
    int scale = ctx.internal_scale.load();
    ImGui::SetNextItemWidth(80);
    if (ImGui::InputInt("Scale", &scale)) {
        scale = std::clamp(scale, 1, 16);
        ctx.internal_scale.store(scale);
    }
    ImGui::SameLine();
    ImGui::TextDisabled("(%dx%d)", DebugContext::BASE_W * scale, DebugContext::BASE_H * scale);

    bool wf = ctx.wireframe.load();
    if (ImGui::Checkbox("Wireframe", &wf))
        ctx.wireframe.store(wf);

    // --- Connection ---
    ImGui::SeparatorText("Connection");
    ImGui::Text("State: %s", conn_state_str(s.conn_state));
    if (s.conn_state > 0) {
        if (!s.server_software.empty()) {
            if (!s.server_version.empty())
                ImGui::Text("Server: %s %s", s.server_software.c_str(), s.server_version.c_str());
            else
                ImGui::Text("Server: %s", s.server_software.c_str());
        }
        ImGui::Text("Players: %d / %d", s.current_players, s.max_players);
    }

    // --- HTTP Streaming ---
    if (s.http_cached > 0 || s.http_pending > 0 || s.http_pool_pending > 0) {
        ImGui::SeparatorText("HTTP Streaming");
        char hbuf[64];
        ImGui::Text("Queue: %d | In-flight: %d | Failed: %d", s.http_pool_pending, s.http_pending, s.http_failed);
        ImGui::Text("Raw cache: %d files, %s", s.http_cached,
                    format_bytes(s.http_cached_bytes, hbuf, sizeof(hbuf)));
    }

    // --- Asset Cache ---
    ImGui::SeparatorText("Asset Cache");
    ImGui::Text("%zu entries | %s / %s", s.cache_entries.size(),
                format_bytes(s.cache_used_bytes, buf, sizeof(buf)),
                format_bytes(s.cache_max_bytes, buf2, sizeof(buf2)));

    // Sort buttons
    auto sort_btn = [&](const char* label, CacheSortMode mode) {
        bool active = (cache_sort_ == mode);
        if (active)
            ImGui::PushStyleColor(ImGuiCol_Button, ImGui::GetStyle().Colors[ImGuiCol_ButtonActive]);
        if (ImGui::SmallButton(label))
            cache_sort_ = mode;
        if (active)
            ImGui::PopStyleColor();
    };
    ImGui::SameLine();
    sort_btn("Name", SORT_NAME);
    ImGui::SameLine();
    sort_btn("LRU", SORT_LRU);
    ImGui::SameLine();
    sort_btn("Refs", SORT_REFS);
    ImGui::SameLine();
    sort_btn("Size", SORT_SIZE);

    // Sort entries (cache_entries arrives in LRU order from snapshot_lru)
    auto& entries = s.cache_entries;
    switch (cache_sort_) {
    case SORT_NAME:
        std::sort(entries.begin(), entries.end(),
                  [](const auto& a, const auto& b) { return a.path < b.path; });
        break;
    case SORT_LRU:
        // Already in LRU order from snapshot_lru; alphabetical tiebreaker
        // isn't possible here since LRU position is the primary key.
        // Just keep the snapshot order.
        break;
    case SORT_REFS:
        std::sort(entries.begin(), entries.end(),
                  [](const auto& a, const auto& b) {
                      if (a.use_count != b.use_count) return a.use_count > b.use_count;
                      return a.path < b.path;
                  });
        break;
    case SORT_SIZE:
        std::sort(entries.begin(), entries.end(),
                  [](const auto& a, const auto& b) {
                      if (a.bytes != b.bytes) return a.bytes > b.bytes;
                      return a.path < b.path;
                  });
        break;
    }

    // Resolve path-based selection to current index
    selected_cache_entry_ = -1;
    if (!selected_cache_path_.empty()) {
        for (int i = 0; i < (int)entries.size(); i++) {
            if (entries[i].path == selected_cache_path_) {
                selected_cache_entry_ = i;
                break;
            }
        }
    }

    // Two-column layout: list on left, preview on right
    float avail_w = ImGui::GetContentRegionAvail().x;
    float preview_w = 140.0f;
    float list_w = avail_w - preview_w - ImGui::GetStyle().ItemSpacing.x;
    float section_h = 200.0f;

    ImGui::BeginChild("##cache_list", ImVec2(list_w, section_h), ImGuiChildFlags_Borders);
    for (int i = 0; i < (int)entries.size(); i++) {
        const auto& e = entries[i];
        char label[256];
        char size_buf2[64];
        format_bytes(e.bytes, size_buf2, sizeof(size_buf2));
        std::snprintf(label, sizeof(label), "%s [%s] %s (refs:%ld)", e.path.c_str(), e.format.c_str(), size_buf2, e.use_count);
        if (ImGui::Selectable(label, selected_cache_entry_ == i)) {
            selected_cache_entry_ = i;
            selected_cache_path_ = e.path;
        }
    }
    ImGui::EndChild();

    ImGui::SameLine();

    ImGui::BeginChild("##cache_preview", ImVec2(preview_w, section_h), ImGuiChildFlags_Borders);
    if (selected_cache_entry_ >= 0 && selected_cache_entry_ < (int)s.cache_entries.size()) {
        const auto& e = s.cache_entries[selected_cache_entry_];

        if (e.texture_id != 0 && e.width > 0 && e.height > 0) {
            float aspect = (float)e.width / (float)e.height;
            float img_w = preview_w - 8.0f;
            float img_h = img_w / aspect;
            if (img_h > section_h * 0.6f) {
                img_h = section_h * 0.6f;
                img_w = img_h * aspect;
            }
            ImTextureID tex = (ImTextureID)e.texture_id;
            ImVec2 uv0(0, 1);
            ImVec2 uv1(1, 0);
            ImGui::Image(tex, ImVec2(img_w, img_h), uv0, uv1);

            // Hover: show at native resolution in a tooltip
            if (ImGui::IsItemHovered()) {
                ImGui::BeginTooltip();
                ImGui::Image(tex, ImVec2((float)e.width, (float)e.height), uv0, uv1);
                ImGui::EndTooltip();
            }
        } else {
            ImGui::TextDisabled("(no preview)");
        }

        ImGui::Separator();
        ImGui::TextWrapped("%s", e.path.c_str());
        ImGui::Text("Format: %s", e.format.c_str());
        if (e.width > 0)
            ImGui::Text("Size: %dx%d", e.width, e.height);
        if (e.frame_count > 1)
            ImGui::Text("Frames: %d", e.frame_count);
        char sb[64];
        format_bytes(e.bytes, sb, sizeof(sb));
        ImGui::Text("Memory: %s", sb);
        ImGui::Text("Refs: %ld", e.use_count);
    } else {
        ImGui::TextDisabled("Select an entry");
    }
    ImGui::EndChild();

    if (!s.http_cache_entries.empty() && ImGui::TreeNode("HTTP Raw Cache")) {
        ImGui::BeginChild("##http_cache_list", ImVec2(0, 150), ImGuiChildFlags_Borders);
        for (const auto& e : s.http_cache_entries) {
            char sb[64];
            format_bytes(e.bytes, sb, sizeof(sb));
            ImGui::Text("%s  %s", sb, e.path.c_str());
        }
        ImGui::EndChild();
        ImGui::TreePop();
    }

    // --- Log ---
    ImGui::SeparatorText("Log");
    draw_log();
}

void DebugOverlayWidget::draw_history() {
    auto& s = stats_;
    if (s.tick_sections.empty())
        return;

    // Low-pass filter coefficient (0 = no filtering, 1 = infinite smoothing)
    static constexpr float LPF_ALPHA = 0.85f;

    // Find smoothed max across all sections for a stable Y scale
    float max_us = 1.0f;
    for (const auto& sec : s.tick_sections) {
        auto it = tick_history_.find(sec.name);
        if (it == tick_history_.end()) continue;
        const auto& ring = it->second;
        float smoothed = 0;
        for (int i = 0; i < ring.count; i++) {
            smoothed = LPF_ALPHA * smoothed + (1.0f - LPF_ALPHA) * ring.at(i);
            max_us = std::max(max_us, smoothed);
        }
    }
    max_us = std::ceil(max_us / 100.0f) * 100.0f;

    // Fixed legend at the top
    int color_idx = 0;
    for (const auto& sec : s.tick_sections) {
        ImU32 col = SLICE_COLORS[color_idx % NUM_COLORS];
        if (color_idx > 0) ImGui::SameLine();
        ImGui::PushStyleColor(ImGuiCol_Text, col);
        float avg = tick_avg_.count(sec.name) ? tick_avg_[sec.name].average() : 0;
        ImGui::Text("%s: %.0f us", sec.name, avg);
        ImGui::PopStyleColor();
        color_idx++;
    }

    ImVec2 avail = ImGui::GetContentRegionAvail();
    float graph_h = std::max(avail.y - 4.0f, 60.0f);

    ImVec2 origin = ImGui::GetCursorScreenPos();
    ImGui::InvisibleButton("##history_area", ImVec2(avail.x, graph_h));
    ImDrawList* dl = ImGui::GetWindowDrawList();

    dl->AddRectFilled(origin, {origin.x + avail.x, origin.y + graph_h}, IM_COL32(30, 30, 30, 200));

    for (int g = 1; g <= 3; g++) {
        float y = origin.y + graph_h * (1.0f - (float)g / 4.0f);
        dl->AddLine({origin.x, y}, {origin.x + avail.x, y}, IM_COL32(60, 60, 60, 255));
        char label[32];
        std::snprintf(label, sizeof(label), "%.0f us", max_us * (float)g / 4.0f);
        dl->AddText({origin.x + 2, y - 12}, IM_COL32(120, 120, 120, 255), label);
    }

    // Draw each section as a smoothed line
    color_idx = 0;
    for (const auto& sec : s.tick_sections) {
        auto it = tick_history_.find(sec.name);
        if (it == tick_history_.end()) continue;
        const auto& ring = it->second;
        if (ring.count < 2) { color_idx++; continue; }

        ImU32 col = SLICE_COLORS[color_idx % NUM_COLORS];
        color_idx++;

        int n = ring.count;
        float x_step = avail.x / (float)(HISTORY_SIZE - 1);
        float x_offset = (HISTORY_SIZE - n) * x_step;

        float smoothed = ring.at(0);
        ImVec2 prev = {origin.x + x_offset, origin.y + graph_h * (1.0f - smoothed / max_us)};
        for (int i = 1; i < n; i++) {
            smoothed = LPF_ALPHA * smoothed + (1.0f - LPF_ALPHA) * ring.at(i);
            float x = origin.x + x_offset + i * x_step;
            float y = origin.y + graph_h * (1.0f - smoothed / max_us);
            ImVec2 pt = {x, y};
            dl->AddLine(prev, pt, col, 1.0f);
            prev = pt;
        }
    }
}

static ImVec4 log_level_color(LogLevel level) {
    switch (level) {
    case VERBOSE: return {0.5f, 0.5f, 0.5f, 1.0f};
    case DEBUG:   return {0.6f, 0.6f, 0.6f, 1.0f};
    case INFO:    return {0.8f, 0.8f, 0.8f, 1.0f};
    case WARNING: return {1.0f, 0.8f, 0.2f, 1.0f};
    case ERR:     return {1.0f, 0.3f, 0.3f, 1.0f};
    case FATAL:   return {1.0f, 0.0f, 0.0f, 1.0f};
    default:      return {1.0f, 1.0f, 1.0f, 1.0f};
    }
}

void DebugOverlayWidget::draw_log() {
    // Level filter buttons
    static const char* filter_names[] = {"", "VRB", "DBG", "INF", "WRN", "ERR", "FTL"};
    for (int i = VERBOSE; i < COUNT; i++) {
        if (i > VERBOSE) ImGui::SameLine();
        ImGui::PushStyleColor(ImGuiCol_Text, log_level_color((LogLevel)i));
        ImGui::Checkbox(filter_names[i], &log_filter_[i]);
        ImGui::PopStyleColor();
    }

    ImGui::SameLine();
    ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
    ImGui::InputTextWithHint("##log_search", "Search...", log_search_, sizeof(log_search_));

    // Poll only new entries since last frame
    log_gen_ = LogBuffer::instance().poll(log_gen_, log_local_);

    if (ImGui::SmallButton("Copy")) {
        std::string text;
        for (const auto& entry : log_local_) {
            if (!log_filter_[entry.level]) continue;
            if (log_search_[0] != '\0' && entry.message.find(log_search_) == std::string::npos) continue;
            text += '[';
            text += entry.timestamp;
            text += "][";
            text += log_level_name(entry.level);
            text += "] ";
            text += entry.message;
            text += '\n';
        }
        ImGui::SetClipboardText(text.c_str());
    }
    ImGui::SameLine();
    if (ImGui::SmallButton("Clear")) {
        LogBuffer::instance().clear();
        log_local_.clear();
        log_gen_ = 0;
    }

    ImGui::BeginChild("##log_scroll", ImVec2(0, 0), ImGuiChildFlags_None);

    for (const auto& entry : log_local_) {
        if (!log_filter_[entry.level])
            continue;
        if (log_search_[0] != '\0' && entry.message.find(log_search_) == std::string::npos)
            continue;

        ImGui::PushStyleColor(ImGuiCol_Text, log_level_color(entry.level));
        ImGui::TextWrapped("[%s][%s] %s", entry.timestamp.c_str(),
                          log_level_name(entry.level), entry.message.c_str());
        ImGui::PopStyleColor();
    }

    if (ImGui::GetScrollY() >= ImGui::GetScrollMaxY() - 10.0f)
        ImGui::SetScrollHereY(1.0f);

    ImGui::EndChild();
}

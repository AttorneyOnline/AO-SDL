#include "ui/widgets/DebugOverlayWidget.h"

#include "render/RenderManager.h"
#include "ui/DebugContext.h"

#include <imgui.h>

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
        if (frac < 0.005f) {
            angle += frac * PI2;
            continue;
        }

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
        if (pct < 0.5f) continue;

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
    for (const auto& sec : s.tick_sections)
        tick_avg_[sec.name].push(sec.us);
    frame_count_++;

    // --- Performance ---
    ImGui::SeparatorText("Performance");
    ImGui::Text("FPS: %.0f  |  Frame: %.2f ms", s.fps, s.frame_time_ms);
    ImGui::Text("Game tick: %.3f ms @ %.0f Hz", s.game_tick_ms, s.tick_rate_hz);
    ImGui::Text("Draw calls: %d", s.draw_calls);

    // --- Game tick breakdown pie ---
    if (!s.tick_sections.empty() && frame_count_ >= 5) {
        ImGui::SeparatorText("Tick Breakdown");
        std::vector<std::pair<const char*, float>> slices;
        for (const auto& sec : s.tick_sections)
            slices.push_back({sec.name, tick_avg_[sec.name].average()});
        draw_pie(slices);
    }

    // --- Renderer ---
    ImGui::SeparatorText("Renderer");
    ImGui::Text("Backend: %s", s.gpu_backend);

    auto& ctx = DebugContext::instance();
    int scale = ctx.internal_scale;
    ImGui::SetNextItemWidth(80);
    if (ImGui::InputInt("Scale", &scale)) {
        scale = std::clamp(scale, 1, 16);
        if (scale != ctx.internal_scale) {
            ctx.internal_scale = scale;
            if (ctx.render_manager) {
                int w = DebugContext::BASE_W * scale;
                int h = DebugContext::BASE_H * scale;
                ctx.render_manager->get_renderer().resize(w, h);
            }
        }
    }
    ImGui::SameLine();
    ImGui::TextDisabled("(%dx%d)", DebugContext::BASE_W * ctx.internal_scale,
                        DebugContext::BASE_H * ctx.internal_scale);

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

    // --- Asset Cache ---
    ImGui::SeparatorText("Asset Cache");
    ImGui::Text("%zu entries | %s / %s", s.cache_entries.size(),
                format_bytes(s.cache_used_bytes, buf, sizeof(buf)),
                format_bytes(s.cache_max_bytes, buf2, sizeof(buf2)));

    if (ImGui::TreeNode("Entries")) {
        for (const auto& e : s.cache_entries) {
            char size_buf[64];
            format_bytes(e.bytes, size_buf, sizeof(size_buf));
            ImGui::BulletText("%s [%s] %s (refs: %ld)", e.path.c_str(), e.format.c_str(), size_buf, e.use_count);
        }
        ImGui::TreePop();
    }
}

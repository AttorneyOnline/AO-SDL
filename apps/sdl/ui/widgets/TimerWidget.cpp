#include "ui/widgets/TimerWidget.h"

#include "event/EventManager.h"
#include "event/TimerEvent.h"

#include <imgui.h>

#include <cstdio>

void TimerWidget::handle_events() {
    auto& ch = EventManager::instance().get_channel<TimerEvent>();
    while (auto ev = ch.get_event()) {
        int id = ev->timer_id();
        if (id < 0 || id >= MAX_TIMERS)
            continue;

        auto& t = timers_[id];
        switch (ev->action()) {
        case 0: // start/sync
            if (ev->time_ms() < 0) {
                t.running = false;
                t.remaining_ms = 0;
            } else {
                t.remaining_ms = ev->time_ms();
                t.running = true;
                t.last_tick = std::chrono::steady_clock::now();
            }
            break;
        case 1: // pause
            t.running = false;
            t.remaining_ms = ev->time_ms();
            break;
        case 2: // show
            t.visible = true;
            break;
        case 3: // hide
            t.visible = false;
            break;
        }
    }
}

void TimerWidget::render() {
    auto now = std::chrono::steady_clock::now();

    bool any_visible = false;
    for (int i = 0; i < MAX_TIMERS; i++) {
        auto& t = timers_[i];
        if (!t.visible)
            continue;
        any_visible = true;

        // Tick down
        if (t.running) {
            auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - t.last_tick).count();
            t.last_tick = now;
            t.remaining_ms -= elapsed;
            if (t.remaining_ms < 0)
                t.remaining_ms = 0;
        }

        int64_t ms = t.remaining_ms;
        int hours = (int)(ms / 3600000);
        int minutes = (int)((ms % 3600000) / 60000);
        int seconds = (int)((ms % 60000) / 1000);
        int centis = (int)((ms % 1000) / 10);

        char buf[32];
        if (hours > 0)
            std::snprintf(buf, sizeof(buf), "%d:%02d:%02d.%02d", hours, minutes, seconds, centis);
        else
            std::snprintf(buf, sizeof(buf), "%02d:%02d.%02d", minutes, seconds, centis);

        if (!t.running)
            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.6f, 0.6f, 0.6f, 1.0f));

        ImGui::Text("Timer %d: %s", i, buf);

        if (!t.running)
            ImGui::PopStyleColor();
    }

    if (!any_visible)
        ImGui::TextDisabled("No timers");
}

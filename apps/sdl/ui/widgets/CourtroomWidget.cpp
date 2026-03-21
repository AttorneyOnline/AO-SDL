#include "ui/widgets/CourtroomWidget.h"

#include "render/IRenderer.h"
#include "render/RenderManager.h"
#include "ui/DebugContext.h"

#include <imgui.h>

void CourtroomWidget::handle_events() {
}

void CourtroomWidget::render() {
    // Poll for resolution scale changes (set by debug widget on UI thread)
    int scale = DebugContext::instance().internal_scale.load();
    int target_w = DebugContext::BASE_W * scale;
    int target_h = DebugContext::BASE_H * scale;
    render_.get_renderer().resize(target_w, target_h);

    render_.render_frame();
    render_.begin_frame();

    IRenderer& renderer = render_.get_renderer();
    ImVec2 uv0 = renderer.uv_flipped() ? ImVec2(0, 1) : ImVec2(0, 0);
    ImVec2 uv1 = renderer.uv_flipped() ? ImVec2(1, 0) : ImVec2(1, 1);

    ImVec2 avail = ImGui::GetContentRegionAvail();

    // Letterbox/pillarbox to maintain 4:3 aspect ratio
    constexpr float target_aspect = 4.0f / 3.0f;
    float avail_aspect = avail.x / std::max(avail.y, 1.0f);

    float img_w, img_h;
    if (avail_aspect > target_aspect) {
        // Wider than 4:3 — pillarbox (bars on sides)
        img_h = avail.y;
        img_w = avail.y * target_aspect;
    } else {
        // Taller than 4:3 — letterbox (bars on top/bottom)
        img_w = avail.x;
        img_h = avail.x / target_aspect;
    }

    ImVec2 cursor = ImGui::GetCursorPos();
    float pad_x = (avail.x - img_w) * 0.5f;
    float pad_y = (avail.y - img_h) * 0.5f;
    ImGui::SetCursorPos(ImVec2(cursor.x + pad_x, cursor.y + pad_y));

    ImTextureID tex = (ImTextureID)renderer.get_display_texture_id((int)img_w, (int)img_h);
    ImGui::Image(tex, ImVec2(img_w, img_h), uv0, uv1);
}

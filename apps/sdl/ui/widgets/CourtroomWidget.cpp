#include "ui/widgets/CourtroomWidget.h"

#include "render/IRenderer.h"
#include "render/RenderManager.h"

#include <imgui.h>

void CourtroomWidget::handle_events() {
}

void CourtroomWidget::render() {
    render_.render_frame();
    render_.begin_frame();

    IRenderer& renderer = render_.get_renderer();
    ImVec2 uv0 = renderer.uv_flipped() ? ImVec2(0, 1) : ImVec2(0, 0);
    ImVec2 uv1 = renderer.uv_flipped() ? ImVec2(1, 0) : ImVec2(1, 1);

    ImGui::Begin("Courtroom");
    ImVec2 avail = ImGui::GetContentRegionAvail();
    ImTextureID tex = (ImTextureID)renderer.get_display_texture_id((int)avail.x, (int)avail.y);
    ImGui::Image(tex, avail, uv0, uv1);
    ImGui::End();
}

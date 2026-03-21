#include "ui/controllers/CourtroomController.h"

#include "ao/ui/screens/CourtroomScreen.h"
#include "asset/MediaManager.h"
#include "event/EventManager.h"
#include "event/PlayerCountEvent.h"
#include "event/ServerInfoEvent.h"
#include "game/GameThread.h"
#include "game/IScenePresenter.h"
#include "render/RenderManager.h"
#include "ui/DebugContext.h"

#include <imgui.h>

static int side_to_index(const std::string& side) {
    static constexpr const char* SIDES[] = {"def", "pro", "wit", "jud", "jur", "sea", "hlp"};
    for (int i = 0; i < 7; i++) {
        if (side == SIDES[i])
            return i;
    }
    return 2; // default: wit
}

CourtroomController::CourtroomController(CourtroomScreen& screen, RenderManager& render)
    : render_(&render) {
    ic_state_ = {};
    ic_state_.character = screen.get_character_name();
    ic_state_.char_id = screen.get_char_id();

    const ICharacterSheet* sheet = screen.get_character_sheet();
    if (sheet) {
        // Share ownership with the screen — the sheet outlives the controller
        ic_state_.char_sheet = std::shared_ptr<const ICharacterSheet>(
            std::shared_ptr<const ICharacterSheet>{}, sheet);
        ic_state_.side_index = side_to_index(sheet->side());
        std::strncpy(ic_state_.showname, sheet->showname().c_str(),
                     sizeof(ic_state_.showname) - 1);
    }

    const auto& icons = screen.get_emote_icons();
    int emote_count = sheet ? sheet->emote_count() : 0;
    for (int i = 0; i < emote_count; i++) {
        EmoteIcon icon;
        icon.comment = sheet->emote(i).comment;

        if (i < (int)icons.size() && icons[i] && icons[i]->frame_count() > 0) {
            const ImageFrame& frame = icons[i]->frame(0);
            icon.icon.emplace(frame.width, frame.height, frame.pixels.data(), 4);
        }

        ic_state_.emote_icons.push_back(std::move(icon));
    }

    courtroom_ = std::make_unique<CourtroomWidget>(render);
    ic_chat_ = std::make_unique<ICChatWidget>(&ic_state_);
    emote_selector_ = std::make_unique<EmoteSelectorWidget>(&ic_state_);
    interjection_ = std::make_unique<InterjectionWidget>(&ic_state_);
    side_select_ = std::make_unique<SideSelectWidget>(&ic_state_);
    message_options_ = std::make_unique<MessageOptionsWidget>(&ic_state_);
    music_area_ = std::make_unique<MusicAreaWidget>(&ic_state_);
}

void CourtroomController::update_debug_stats() {
    auto& s = debug_.stats();
    auto& ctx = DebugContext::instance();

    const auto& io = ImGui::GetIO();
    s.frame_time_ms = io.DeltaTime * 1000.0f;
    s.fps = io.Framerate;

    if (ctx.game_thread) {
        s.game_tick_ms = ctx.game_thread->last_tick_us() / 1000.0f;
        s.tick_rate_hz = ctx.game_thread->tick_rate_hz();
    }

    if (ctx.presenter) {
        auto profile = ctx.presenter->tick_profile();
        s.tick_sections.clear();
        for (const auto& entry : profile)
            s.tick_sections.push_back({entry.name, static_cast<float>(entry.us->load(std::memory_order_relaxed))});
    }

    s.gpu_backend = render_->get_renderer().backend_name();
    s.draw_calls = render_->get_renderer().last_draw_calls();

    const auto& cache = MediaManager::instance().assets().cache();
    s.cache_used_bytes = cache.used_bytes();
    s.cache_max_bytes = cache.max_bytes();
    s.cache_entries.clear();
    for (const auto& e : cache.snapshot()) {
        s.cache_entries.push_back({e.path, e.format, e.bytes, e.use_count});
    }

    // If we're in the courtroom, we're joined
    if (s.conn_state < 2)
        s.conn_state = 2;

    auto& server_ch = EventManager::instance().get_channel<ServerInfoEvent>();
    while (auto ev = server_ch.get_event()) {
        s.server_software = ev->get_software();
        s.server_version = ev->get_version();
    }

    auto& player_ch = EventManager::instance().get_channel<PlayerCountEvent>();
    while (auto ev = player_ch.get_event()) {
        s.current_players = ev->get_current();
        s.max_players = ev->get_max();
    }
}

void CourtroomController::render() {
    const ImGuiViewport* vp = ImGui::GetMainViewport();
    ImGui::SetNextWindowPos(vp->WorkPos);
    ImGui::SetNextWindowSize(vp->WorkSize);
    ImGui::Begin("##courtroom_screen", nullptr,
                 ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove |
                 ImGuiWindowFlags_NoBringToFrontOnFocus);

    ImVec2 avail = ImGui::GetContentRegionAvail();
    float right_width = 280.0f;
    float left_width = avail.x - right_width - ImGui::GetStyle().ItemSpacing.x;
    float top_height = avail.y * 0.65f;
    float bottom_height = avail.y - top_height - ImGui::GetStyle().ItemSpacing.y;

    // === Top row ===

    ImGui::BeginChild("##viewport", ImVec2(left_width, top_height), ImGuiChildFlags_Borders);
    courtroom_->render();
    ImGui::EndChild();

    ImGui::SameLine();

    ImGui::BeginChild("##right_panel", ImVec2(right_width, top_height), ImGuiChildFlags_Borders);
    if (ImGui::BeginTabBar("##right_tabs")) {
        if (ImGui::BeginTabItem("Emotes")) {
            emote_selector_->render();
            ImGui::EndTabItem();
        }
        if (ImGui::BeginTabItem("Music")) {
            music_area_->handle_events();
            music_area_->render();
            ImGui::EndTabItem();
        }
        ImGui::EndTabBar();
    }
    ImGui::EndChild();

    // === Bottom row ===

    float ic_controls_width = left_width * 0.55f;
    float ooc_width = left_width - ic_controls_width - ImGui::GetStyle().ItemSpacing.x;

    ImGui::BeginChild("##ic_controls", ImVec2(ic_controls_width, bottom_height), ImGuiChildFlags_Borders);
    ImGui::SeparatorText("IC Chat");
    ic_chat_->render();

    ImGui::Spacing();
    ImGui::SeparatorText("Interjections");
    interjection_->render();

    ImGui::Spacing();
    ImGui::SeparatorText("Position");
    side_select_->render();

    ImGui::Spacing();
    ImGui::SeparatorText("Options");
    message_options_->render();

    ImGui::Spacing();
    if (ImGui::Button("Change Character"))
        nav_action_ = IUIRenderer::NavAction::POP_SCREEN;
    ImGui::SameLine();
    if (ImGui::Button("Disconnect"))
        nav_action_ = IUIRenderer::NavAction::POP_TO_ROOT;
    ImGui::EndChild();

    ImGui::SameLine();

    ImGui::BeginChild("##chat_panel", ImVec2(ooc_width, bottom_height), ImGuiChildFlags_Borders);
    if (ImGui::BeginTabBar("##chat_tabs")) {
        if (ImGui::BeginTabItem("IC Log")) {
            ic_log_.handle_events();
            ic_log_.render();
            ImGui::EndTabItem();
        }
        if (ImGui::BeginTabItem("OOC Chat")) {
            chat_.handle_events();
            chat_.render();
            ImGui::EndTabItem();
        }
        ImGui::EndTabBar();
    }
    ImGui::EndChild();

    ImGui::SameLine();

    // Bottom-right: debug panel (docked) or toggle button
    if (!debug_floating_) {
        ImGui::BeginChild("##debug", ImVec2(right_width, bottom_height), ImGuiChildFlags_Borders);
        if (ImGui::SmallButton("Pop Out")) debug_floating_ = true;
        ImGui::SameLine();
        ImGui::SeparatorText("Debug");
        update_debug_stats();
        debug_.render();
        ImGui::EndChild();
    } else {
        ImGui::BeginChild("##debug_placeholder", ImVec2(right_width, bottom_height), ImGuiChildFlags_Borders);
        if (ImGui::Button("Show Debug Panel"))
            debug_floating_ = false;
        ImGui::EndChild();
    }

    ImGui::End();

    // Floating debug window
    if (debug_floating_ && debug_open_) {
        ImGui::SetNextWindowSize(ImVec2(320, 500), ImGuiCond_FirstUseEver);
        ImGui::Begin("Debug", &debug_open_);
        if (ImGui::SmallButton("Dock")) debug_floating_ = false;
        update_debug_stats();
        debug_.render();
        ImGui::End();

        if (!debug_open_) {
            debug_open_ = true;
            debug_floating_ = false;
        }
    }
}

IUIRenderer::NavAction CourtroomController::nav_action() {
    auto action = nav_action_;
    nav_action_ = IUIRenderer::NavAction::NONE;
    return action;
}

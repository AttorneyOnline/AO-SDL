#include "ui/controllers/CourtroomController.h"

#include "ao/asset/AOAssetLibrary.h"
#include "ao/ui/screens/CourtroomScreen.h"
#include "asset/MediaManager.h"
#include "asset/MountManager.h"
#include "audio/SDLAudioDevice.h"
#include "event/EventManager.h"
#include "event/PlayerCountEvent.h"
#include "event/ServerInfoEvent.h"
#include "game/GameThread.h"
#include "game/IScenePresenter.h"
#include "render/RenderManager.h"
#include "ui/DebugContext.h"
#include "ui/widgets/CourtroomState.h"
#include "utils/Log.h"

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
    : render_(&render), screen_(screen) {
    ic_state_ = {};
    ic_state_.character = screen.get_character_name();
    ic_state_.char_id = screen.get_char_id();

    // Character data may not be ready yet (async loading).
    // apply_character_data() will be called from render() once it's available.
    if (!screen.is_loading()) {
        apply_character_data();
        auto& ctx = DebugContext::instance();
        if (ctx.presenter)
            ctx.presenter->set_courtroom_active(true);
    }

    courtroom_ = std::make_unique<CourtroomWidget>(render);
    ic_chat_ = std::make_unique<ICChatWidget>(&ic_state_);
    emote_selector_ = std::make_unique<EmoteSelectorWidget>(&ic_state_);
    interjection_ = std::make_unique<InterjectionWidget>(&ic_state_);
    side_select_ = std::make_unique<SideSelectWidget>(&ic_state_);
    message_options_ = std::make_unique<MessageOptionsWidget>(&ic_state_);
    music_area_ = std::make_unique<MusicAreaWidget>(&ic_state_);
}

CourtroomController::~CourtroomController() {
    auto& ctx = DebugContext::instance();
    if (ctx.presenter)
        ctx.presenter->set_courtroom_active(false);
}

void CourtroomController::apply_character_data() {
    ic_state_.character = screen_.get_character_name();
    ic_state_.char_id = screen_.get_char_id();

    auto sheet = screen_.get_character_sheet();
    if (sheet) {
        ic_state_.char_sheet = sheet;
        ic_state_.side_index = side_to_index(sheet->side());
        std::strncpy(ic_state_.showname, sheet->showname().c_str(), sizeof(ic_state_.showname) - 1);
    }

    ic_state_.emote_icons.clear();
    const auto& icons = screen_.get_emote_icons();
    int emote_count = sheet ? sheet->emote_count() : 0;
    for (int i = 0; i < emote_count; i++) {
        EmoteIcon icon;
        icon.comment = sheet->emote(i).comment;

        if (i < (int)icons.size() && icons[i] && icons[i]->frame_count() > 0) {
            const ImageFrame& frame = icons[i]->frame(0);
            icon.icon.emplace(frame.width, frame.height, icons[i]->frame_pixels(0), 4);
        }

        ic_state_.emote_icons.push_back(std::move(icon));
    }

    // Auto-set Pre checkbox for the default emote
    ic_state_.selected_emote = 0;
    if (sheet && emote_count > 0) {
        const auto& emo = sheet->emote(0);
        ic_state_.pre_anim = !emo.pre_anim.empty() && emo.pre_anim != "-";
    }

    last_load_gen_ = screen_.load_generation();
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
    s.uv_flipped = render_->get_renderer().uv_flipped();

    auto& assets = MediaManager::instance().assets();
    const auto& cache = assets.cache();
    s.cache_used_bytes = cache.used_bytes();
    s.cache_max_bytes = cache.max_bytes();

    // Only resample the full cache entry list every 2 seconds to avoid flicker
    auto now = std::chrono::steady_clock::now();
    if (s.cache_entries.empty() ||
        std::chrono::duration_cast<std::chrono::milliseconds>(now - cache_sample_time_).count() >= 2000) {
        cache_sample_time_ = now;
        s.cache_entries.clear();
        auto selected_path = debug_.selected_cache_path();
        for (const auto& e : cache.snapshot_lru()) {
            DebugStats::CacheEntry entry{e.path, e.format, e.bytes, e.use_count};
            // Only fetch the ImageAsset for the selected entry (for preview).
            // Holding shared_ptrs to all entries would pin them and prevent eviction.
            if (e.path == selected_path) {
                auto cached = cache.peek(e.path);
                auto img = std::dynamic_pointer_cast<ImageAsset>(cached);
                if (img) {
                    entry.width = img->width();
                    entry.height = img->height();
                    entry.frame_count = img->frame_count();
                    entry.image = img;
                    if (entry.texture_id == 0)
                        entry.texture_id = render_->get_renderer().get_texture_id(img);
                }
            }
            s.cache_entries.push_back(std::move(entry));
        }
    }

    // If we're in the courtroom, we're joined
    if (s.conn_state < 2)
        s.conn_state = 2;

    auto& server_ch = EventManager::instance().get_channel<ServerInfoEvent>();
    while (auto ev = server_ch.get_event()) {
        s.server_software = ev->get_software();
        s.server_version = ev->get_version();
        CourtroomState::instance().local_player_id = ev->get_player_num();
    }

    auto& player_ch = EventManager::instance().get_channel<PlayerCountEvent>();
    while (auto ev = player_ch.get_event()) {
        s.current_players = ev->get_current();
        s.max_players = ev->get_max();
    }

    auto http = MediaManager::instance().mounts_ref().http_stats();
    s.http_pending = http.pending;
    s.http_cached = http.cached;
    s.http_failed = http.failed;
    s.http_pool_pending = http.pool_pending;
    s.http_cached_bytes = http.cached_bytes;

    // Sample HTTP raw cache at the same interval as asset cache
    if (s.cache_entries.empty() ||
        std::chrono::duration_cast<std::chrono::milliseconds>(now - cache_sample_time_).count() < 100) {
        // Reuse the sample timing — http entries update alongside asset cache
        s.http_cache_entries.clear();
        for (const auto& e : MediaManager::instance().mounts_ref().http_cache_snapshot())
            s.http_cache_entries.push_back({e.path, e.bytes});
    }

    // Event channel stats
    auto channel_stats = EventManager::instance().snapshot_channel_stats();
    s.event_stats.clear();
    s.event_stats.reserve(channel_stats.size());
    for (const auto& cs : channel_stats)
        s.event_stats.push_back({cs.raw_name, cs.count});

    // Audio channel stats
    s.audio_channels.clear();
    if (ctx.audio_device) {
        for (const auto& ci : ctx.audio_device->channel_snapshot()) {
            s.audio_channels.push_back({ci.id, ci.active, ci.is_stream, ci.stream_ready, ci.stream_finished,
                                        ci.stream_looping, ci.loop_start, ci.loop_end, ci.volume, ci.ring_available});
        }
    }
}

void CourtroomController::retry_emote_icons() {
    if (!ic_state_.char_sheet)
        return;

    AOAssetLibrary ao_assets(MediaManager::instance().assets());
    bool any_missing = false;

    for (int i = 0; i < (int)ic_state_.emote_icons.size(); i++) {
        if (ic_state_.emote_icons[i].icon.has_value())
            continue;
        any_missing = true;

        auto asset = ao_assets.emote_icon(ic_state_.character, i);
        if (asset && asset->frame_count() > 0) {
            const ImageFrame& frame = asset->frame(0);
            ic_state_.emote_icons[i].icon.emplace(frame.width, frame.height, asset->frame_pixels(0), 4);
            Log::log_print(DEBUG, "Retry loaded emote icon %d for %s", i, ic_state_.character.c_str());
        }
    }

    // Stop retrying once all icons are loaded
    (void)any_missing;
}

void CourtroomController::render() {
    // Check if async character data loading has finished
    if (screen_.load_generation() != last_load_gen_ && !screen_.is_loading()) {
        apply_character_data();
        // Signal presenter that the courtroom is now visible — start audio
        auto& ctx = DebugContext::instance();
        if (ctx.presenter)
            ctx.presenter->set_courtroom_active(true);
    }

    // Show loading overlay while character data is being fetched
    if (screen_.is_loading()) {
        const ImGuiViewport* vp = ImGui::GetMainViewport();
        ImGui::SetNextWindowPos(vp->WorkPos);
        ImGui::SetNextWindowSize(vp->WorkSize);
        ImGui::Begin("##loading", nullptr,
                     ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoBringToFrontOnFocus);
        ImVec2 center(vp->WorkSize.x * 0.5f, vp->WorkSize.y * 0.5f);
        const char* text = "Loading character data...";
        ImVec2 text_size = ImGui::CalcTextSize(text);
        ImGui::SetCursorPos(ImVec2(center.x - text_size.x * 0.5f, center.y - text_size.y * 0.5f));
        ImGui::Text("%s", text);
        ImGui::End();
        return;
    }

    retry_emote_icons();

    const ImGuiViewport* vp = ImGui::GetMainViewport();
    ImGui::SetNextWindowPos(vp->WorkPos);
    ImGui::SetNextWindowSize(vp->WorkSize);
    ImGui::Begin("##courtroom_screen", nullptr,
                 ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoBringToFrontOnFocus |
                     ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);

    health_bar_.handle_events();
    timer_.handle_events();

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
        if (ImGui::BeginTabItem("Evidence")) {
            evidence_.handle_events();
            evidence_.render();
            ImGui::EndTabItem();
        }
        if (ImGui::BeginTabItem("Players")) {
            player_list_.handle_events();
            player_list_.render();
            ImGui::EndTabItem();
        }
        ImGui::EndTabBar();
    }
    ImGui::EndChild();

    // === Bottom row ===

    float ic_controls_width = left_width * 0.55f;

    ImGui::BeginChild("##ic_controls", ImVec2(ic_controls_width, bottom_height), ImGuiChildFlags_Borders);

    // HP bars & timers
    health_bar_.render();
    timer_.render();
    ImGui::Separator();

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

    float remaining_width = avail.x - ic_controls_width - ImGui::GetStyle().ItemSpacing.x;
    ImGui::BeginChild("##chat_panel", ImVec2(remaining_width, bottom_height), ImGuiChildFlags_Borders);
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

    // Toggle debug window via /debug in OOC chat
    if (chat_.consume_debug_toggle())
        debug_open_ = !debug_open_;

    ImGui::End();

    // Keep presenter in sync with local player state (slide checkbox, char_id)
    {
        auto& ctx = DebugContext::instance();
        if (ctx.presenter)
            ctx.presenter->set_local_player(ic_state_.char_id, ic_state_.slide);
    }

    // Floating debug window (toggled by /debug)
    if (debug_open_) {
        ImGui::SetNextWindowSize(ImVec2(420, 600), ImGuiCond_FirstUseEver);
        ImGui::Begin("Debug", &debug_open_);
        update_debug_stats();
        debug_.render();
        ImGui::End();
    }
}

IUIRenderer::NavAction CourtroomController::nav_action() {
    auto action = nav_action_;
    nav_action_ = IUIRenderer::NavAction::NONE;
    return action;
}

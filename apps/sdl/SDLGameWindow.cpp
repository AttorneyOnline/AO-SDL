#include "SDLGameWindow.h"

#include "platform/SystemFonts.h"
#include "utils/Log.h"

#include <imgui.h>
#include <imgui_impl_sdl2.h>

SDLGameWindow::SDLGameWindow(UIManager& ui_manager, std::unique_ptr<IGPUBackend> backend)
    : window(nullptr), ui_manager(ui_manager), gpu(std::move(backend)), running(true) {

    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO) < 0) {
        Log::log_print(LogLevel::FATAL, "Failed to initialize SDL2: %s", SDL_GetError());
    }

    gpu->pre_init(); // Set GL/Metal attributes before window creation

    uint32_t flags = SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE | gpu->window_flags();
    window = SDL_CreateWindow("SDL2", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 1280, 720, flags);
    if (!window) {
        Log::log_print(LogLevel::FATAL, "Failed to create window: %s", SDL_GetError());
    }

    ImGui::CreateContext();
    ImGui::StyleColorsDark();
    ImGui::GetStyle().WindowPadding = ImVec2(0, 0);

    // Load system fonts with wide Unicode coverage into ImGui's atlas.
    // The first font provides the base Latin glyphs; subsequent fonts are
    // merged to cover CJK, Korean, Thai, Cyrillic, etc.
    auto font_paths = platform::fallback_font_paths();
    if (!font_paths.empty()) {
        ImGuiIO& io = ImGui::GetIO();

        // Base font: load the first available system font with default ranges
        bool base_loaded = false;
        for (const auto& path : font_paths) {
            if (io.Fonts->AddFontFromFileTTF(path.c_str(), 15.0f, nullptr, io.Fonts->GetGlyphRangesDefault())) {
                Log::log_print(DEBUG, "ImGui: base UI font from %s", path.c_str());
                base_loaded = true;
                break;
            }
        }
        if (!base_loaded)
            io.Fonts->AddFontDefault();

        // Merge additional glyph ranges from system fonts.
        // Each range is tried against each font until one provides it.
        struct RangeSet {
            const char* name;
            const ImWchar* (*getter)(ImFontAtlas*);
        };
        RangeSet extra_ranges[] = {
            {"Korean", [](ImFontAtlas* a) { return a->GetGlyphRangesKorean(); }},
            {"Chinese", [](ImFontAtlas* a) { return a->GetGlyphRangesChineseFull(); }},
            {"Japanese", [](ImFontAtlas* a) { return a->GetGlyphRangesJapanese(); }},
            {"Cyrillic", [](ImFontAtlas* a) { return a->GetGlyphRangesCyrillic(); }},
            {"Thai", [](ImFontAtlas* a) { return a->GetGlyphRangesThai(); }},
            {"Vietnamese", [](ImFontAtlas* a) { return a->GetGlyphRangesVietnamese(); }},
        };

        // Merge from ALL system fonts so each contributes the glyphs it has.
        // ImGui's atlas deduplicates — a glyph already covered by an earlier
        // font won't be overwritten.
        ImFontGlyphRangesBuilder builder;
        for (auto& rs : extra_ranges)
            builder.AddRanges(rs.getter(io.Fonts));
        ImVector<ImWchar> merged_ranges;
        builder.BuildRanges(&merged_ranges);

        for (const auto& path : font_paths) {
            ImFontConfig merge_cfg;
            merge_cfg.MergeMode = true;
            merge_cfg.OversampleH = 1;
            if (io.Fonts->AddFontFromFileTTF(path.c_str(), 15.0f, &merge_cfg, merged_ranges.Data))
                Log::log_print(DEBUG, "ImGui: merged glyphs from %s", path.c_str());
        }
    }
}

SDLGameWindow::~SDLGameWindow() {
    gpu->shutdown();
    ImGui::DestroyContext();
    if (window)
        SDL_DestroyWindow(window);
    SDL_Quit();
}

void SDLGameWindow::start_loop(RenderManager& render, IUIRenderer& ui_renderer) {
    gpu->init(window, render.get_renderer());

    SDL_Event event;
    while (running) {
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) {
                Log::log_print(DEBUG, "SDLGameWindow: SDL_QUIT received");
                running = false;
            }
            ImGui_ImplSDL2_ProcessEvent(&event);
        }

        gpu->begin_frame();
        if (frame_callback_)
            frame_callback_();
        ui_manager.handle_events();

        Screen* screen = ui_manager.active_screen();
        if (screen) {
            ui_renderer.begin_frame();
            ui_renderer.render_screen(*screen, render);
            ui_renderer.end_frame();
        }

        auto nav = ui_renderer.pending_nav_action();
        if (nav == IUIRenderer::NavAction::POP_TO_ROOT)
            ui_manager.pop_to_root();
        else if (nav == IUIRenderer::NavAction::POP_SCREEN)
            ui_manager.pop_screen();

        gpu->present();
    }
}

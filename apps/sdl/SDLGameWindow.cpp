#include "SDLGameWindow.h"

#include "asset/MountEmbedded.h"
#include "platform/SystemFonts.h"
#include "ui/widgets/CourtroomState.h"
#include "utils/Log.h"

#include <imgui.h>
#include <imgui_impl_sdl2.h>

#include <cstring>
#include <filesystem>

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

    gpu->create_context(window); // GL context / Metal view — must exist before renderer

    ImGui::CreateContext();
    ImGui::StyleColorsDark();
    ImGui::GetStyle().WindowPadding = ImVec2(0, 0);

    // Load system fonts with wide Unicode coverage into ImGui's atlas.
    //
    // ImGui's font system is a static batch: AddFont* reads the entire TTF file
    // into heap memory that must persist for the atlas lifetime (since ImGui 1.92).
    // There is no lazy/on-demand glyph loading. macOS returns ~30 cascade fonts
    // including Apple Color Emoji (180 MB) and PingFang (75 MB), so loading all
    // of them would waste 300+ MB of heap.
    //
    // We skip fonts above a size threshold. The in-game text renderer uses
    // FreeType with mmap (demand-paged, efficient) and on-demand rasterization
    // via GlyphCache, so full Unicode coverage is not lost — only the ImGui UI
    // chrome (menus, settings, debug overlay) is affected.
    auto font_paths = platform::fallback_font_paths();
    if (!font_paths.empty()) {
        ImGuiIO& io = ImGui::GetIO();

        // Skip fonts larger than 30 MB. This excludes:
        //   - Apple Color Emoji.ttc (~180 MB) — ImGui can't render color emoji
        //   - PingFang.ttc (~75 MB) — CJK covered by smaller Hiragino/Korean fonts
        // The remaining ~20 fonts (~80 MB) cover CJK, Korean, Thai, Cyrillic, etc.
        static constexpr uintmax_t MAX_FONT_FILE_SIZE = 30 * 1024 * 1024;

        std::vector<std::string> filtered_paths;
        for (const auto& path : font_paths) {
            std::error_code ec;
            auto size = std::filesystem::file_size(path, ec);
            if (ec)
                continue;
            if (size > MAX_FONT_FILE_SIZE) {
                Log::log_print(DEBUG, "ImGui: skipping oversized font (%juMB): %s",
                               static_cast<uintmax_t>(size / (1024 * 1024)), path.c_str());
                continue;
            }
            filtered_paths.push_back(path);
        }

        // Base font: load the first available system font with default ranges
        bool base_loaded = false;
        for (const auto& path : filtered_paths) {
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

        // Merge from filtered system fonts so each contributes the glyphs it has.
        // ImGui's atlas deduplicates — a glyph already covered by an earlier
        // font won't be overwritten.
        ImFontGlyphRangesBuilder builder;
        for (auto& rs : extra_ranges)
            builder.AddRanges(rs.getter(io.Fonts));
        ImVector<ImWchar> merged_ranges;
        builder.BuildRanges(&merged_ranges);

        for (const auto& path : filtered_paths) {
            ImFontConfig merge_cfg;
            merge_cfg.MergeMode = true;
            merge_cfg.OversampleH = 1;
            if (io.Fonts->AddFontFromFileTTF(path.c_str(), 15.0f, &merge_cfg, merged_ranges.Data))
                Log::log_print(DEBUG, "ImGui: merged glyphs from %s", path.c_str());
        }

        // Merge bundled Noto Emoji font for monochrome emoji support.
        // This is a lightweight (~860 KB) embedded font that replaces the
        // 180 MB Apple Color Emoji we filter out above.
        // Noto Emoji covers Unicode emoji blocks that ImGui's built-in
        // glyph ranges don't include, so we specify them explicitly.
        static const ImWchar emoji_ranges[] = {
            0x2600,  0x27BF,  // Misc Symbols, Dingbats
            0x2B50,  0x2B55,  // Stars, circles
            0xFE00,  0xFE0F,  // Variation selectors
            0x1F300, 0x1F9FF, // Misc Symbols & Pictographs through Supplemental Symbols
            0,
        };
        for (const auto& file : embedded_assets()) {
            if (std::strcmp(file.path, "fonts/NotoEmoji.ttf") == 0) {
                ImFontConfig emoji_cfg;
                emoji_cfg.MergeMode = true;
                emoji_cfg.OversampleH = 1;
                emoji_cfg.FontDataOwnedByAtlas = false; // data is in the binary, don't free
                if (io.Fonts->AddFontFromMemoryTTF(const_cast<void*>(static_cast<const void*>(file.data)),
                                                   static_cast<int>(file.size), 15.0f, &emoji_cfg, emoji_ranges))
                    Log::log_print(INFO, "ImGui: merged bundled Noto Emoji (%zu bytes)", file.size);
                break;
            }
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
    gpu->init_imgui(window, render.get_renderer());

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
        if (nav == IUIRenderer::NavAction::POP_TO_ROOT) {
            CourtroomState::instance().reset();
            ui_manager.pop_to_root();
        }
        else if (nav == IUIRenderer::NavAction::POP_SCREEN)
            ui_manager.pop_screen();

        gpu->present();
    }
}

#include "platform/SystemFonts.h"

#include <cstdlib>
#include <filesystem>

namespace platform {

std::vector<std::string> fallback_font_paths() {
    std::vector<std::string> paths;

    // Windows doesn't expose a cascade list API as cleanly as CoreText.
    // Scan the system font directory for known wide-coverage fonts.
    const char* windir = std::getenv("WINDIR");
    std::string font_dir = windir ? std::string(windir) + "\\Fonts" : "C:\\Windows\\Fonts";

    // Priority order: wide Unicode coverage first, then CJK-specific
    static const char* names[] = {
        "arial.ttf",    "segoeui.ttf",
        "malgun.ttf",   // Korean (Malgun Gothic)
        "meiryo.ttc",   // Japanese
        "msyh.ttc",     // Chinese Simplified (Microsoft YaHei)
        "msjh.ttc",     // Chinese Traditional
        "msgothic.ttc", // Japanese fallback
        "simsun.ttc",   // Chinese fallback
        "seguiemj.ttf", // Emoji
        "yugothic.ttf", // Japanese (Yu Gothic)
    };

    for (const char* name : names) {
        auto p = std::filesystem::path(font_dir) / name;
        if (std::filesystem::exists(p))
            paths.push_back(p.string());
    }

    return paths;
}

} // namespace platform

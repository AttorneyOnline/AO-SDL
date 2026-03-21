#include "platform/SystemFonts.h"

#include <cstdio>
#include <cstring>
#include <memory>

namespace platform {

std::vector<std::string> fallback_font_paths() {
    std::vector<std::string> paths;

    // Use fontconfig CLI to query system fonts sorted by coverage.
    // fc-list returns all installed fonts; we filter for known wide-coverage families.
    // This avoids linking against libfontconfig directly.
    static const char* families[] = {
        "Noto Sans CJK",
        "Noto Sans",
        "DejaVu Sans",
        "Liberation Sans",
        "FreeSans",
        "Droid Sans Fallback",
    };

    for (const char* family : families) {
        char cmd[256];
        std::snprintf(cmd, sizeof(cmd), "fc-match -f '%%{file}\\n' '%s' 2>/dev/null", family);

        std::unique_ptr<FILE, decltype(&pclose)> pipe(popen(cmd, "r"), pclose);
        if (!pipe)
            continue;

        char line[512];
        if (std::fgets(line, sizeof(line), pipe.get())) {
            // Strip trailing newline
            size_t len = std::strlen(line);
            if (len > 0 && line[len - 1] == '\n')
                line[len - 1] = '\0';
            if (line[0] != '\0')
                paths.emplace_back(line);
        }
    }

    return paths;
}

} // namespace platform

#include "platform/SystemFonts.h"

#import <CoreText/CoreText.h>
#import <Foundation/Foundation.h>

#include <unordered_set>

namespace platform {

std::vector<std::string> fallback_font_paths() {
    std::vector<std::string> paths;
    std::unordered_set<std::string> seen;

    @autoreleasepool {
        // Get the system's default font cascade list — this is the OS-native
        // fallback chain used by CoreText for missing glyphs. It covers CJK,
        // Hangul, Arabic, Devanagari, emoji, etc. in the right priority order
        // for the user's locale.
        CTFontRef base = CTFontCreateWithName(CFSTR("Helvetica"), 12.0, NULL);
        if (!base)
            return paths;

        CFArrayRef cascade = CTFontCopyDefaultCascadeListForLanguages(base, NULL);
        CFRelease(base);
        if (!cascade)
            return paths;

        CFIndex count = CFArrayGetCount(cascade);
        for (CFIndex i = 0; i < count; i++) {
            CTFontDescriptorRef desc = (CTFontDescriptorRef)CFArrayGetValueAtIndex(cascade, i);
            if (!desc)
                continue;

            // Create a font from the descriptor to resolve the file path
            CTFontRef font = CTFontCreateWithFontDescriptor(desc, 12.0, NULL);
            if (!font)
                continue;

            CFURLRef url = (CFURLRef)CTFontCopyAttribute(font, kCTFontURLAttribute);
            CFRelease(font);
            if (!url)
                continue;

            char buf[1024];
            if (CFURLGetFileSystemRepresentation(url, true, (UInt8*)buf, sizeof(buf))) {
                std::string path(buf);
                if (seen.insert(path).second)
                    paths.push_back(std::move(path));
            }
            CFRelease(url);
        }

        CFRelease(cascade);
    }

    return paths;
}

} // namespace platform

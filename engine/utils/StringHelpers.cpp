#include <utils/StringHelpers.h>

// Logic ported from AO2. Takes a string with paths and extension (e.g. "AA/Logic/Logic & Trick.opus")
// and returns a cleaner version for humans to read (e.g. "Logic & Trick")
std::string StringHelpers::trim_song_name(const std::string& t) {
    const size_t dir_end_idx = t.find_last_of('/');
    const size_t start = (dir_end_idx == std::string::npos) ? 0 : dir_end_idx + 1;

    const size_t end = t.find_last_of('.');

    return (end == std::string::npos || end <= start) ? t.substr(start) : t.substr(start, end - start);
}

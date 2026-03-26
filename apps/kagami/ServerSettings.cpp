#include "ServerSettings.h"

#include "utils/Log.h"

#include <fstream>
#include <vector>

bool ServerSettings::load_from_disk(const std::string& path) {
    std::ifstream file(path, std::ios::binary);
    if (!file.is_open()) {
        Log::log_print(INFO, "No config file at %s — using defaults", path.c_str());
        return false;
    }

    std::vector<uint8_t> data((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
    if (!instance().deserialize(data)) {
        Log::log_print(WARNING, "Failed to parse %s — using defaults", path.c_str());
        return false;
    }

    Log::log_print(INFO, "Loaded config from %s", path.c_str());
    return true;
}

bool ServerSettings::save_to_disk(const std::string& path) {
    auto data = instance().serialize();
    std::ofstream file(path, std::ios::binary | std::ios::trunc);
    if (!file.is_open()) {
        Log::log_print(WARNING, "Could not write config to %s", path.c_str());
        return false;
    }

    file.write(reinterpret_cast<const char*>(data.data()), data.size());
    Log::log_print(INFO, "Saved config to %s", path.c_str());
    return true;
}

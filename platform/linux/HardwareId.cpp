#include "platform/HardwareId.h"

#include <cstdio>
#include <cstring>
#include <fstream>
#include <iomanip>
#include <sstream>
#include <string>

// Minimal SHA-256 (no OpenSSL dependency). Uses /usr/bin/sha256sum as fallback.
static std::string sha256_hex(const std::string& input) {
    // Try piping through sha256sum (available on virtually all Linux systems)
    FILE* pipe = popen("sha256sum", "w");
    if (!pipe)
        return "";

    // Use a temp approach: write to sha256sum via echo | sha256sum
    std::string cmd = "echo -n '" + input + "' | sha256sum 2>/dev/null";
    std::unique_ptr<FILE, decltype(&pclose)> proc(popen(cmd.c_str(), "r"), pclose);
    if (!proc)
        return "";

    char buf[128] = {};
    if (std::fgets(buf, sizeof(buf), proc.get())) {
        // sha256sum outputs: "<hash>  -\n"
        char* space = std::strchr(buf, ' ');
        if (space)
            *space = '\0';
        return buf;
    }
    return "";
}

static std::string read_file_trimmed(const char* path) {
    std::ifstream f(path);
    std::string line;
    if (std::getline(f, line)) {
        while (!line.empty() && (line.back() == '\n' || line.back() == '\r'))
            line.pop_back();
        return line;
    }
    return "";
}

namespace platform {

std::string hardware_id() {
    // Primary: /etc/machine-id — a systemd-generated unique ID that persists
    // across reboots. Present on virtually all modern Linux distributions.
    std::string machine_id = read_file_trimmed("/etc/machine-id");
    if (!machine_id.empty())
        return sha256_hex("ao-sdl:linux:" + machine_id);

    // Fallback: /var/lib/dbus/machine-id (older systems without systemd)
    machine_id = read_file_trimmed("/var/lib/dbus/machine-id");
    if (!machine_id.empty())
        return sha256_hex("ao-sdl:linux:" + machine_id);

    // Last resort: DMI product UUID (requires root on some systems)
    machine_id = read_file_trimmed("/sys/class/dmi/id/product_uuid");
    if (!machine_id.empty())
        return sha256_hex("ao-sdl:linux:" + machine_id);

    return sha256_hex("ao-sdl:linux:unknown");
}

} // namespace platform

#include "platform/HardwareId.h"

#include "utils/Crypto.h"

#include <fstream>
#include <string>

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
        return crypto::sha256("ao-sdl:linux:" + machine_id);

    // Fallback: /var/lib/dbus/machine-id (older systems without systemd)
    machine_id = read_file_trimmed("/var/lib/dbus/machine-id");
    if (!machine_id.empty())
        return crypto::sha256("ao-sdl:linux:" + machine_id);

    // Last resort: DMI product UUID (requires root on some systems)
    machine_id = read_file_trimmed("/sys/class/dmi/id/product_uuid");
    if (!machine_id.empty())
        return crypto::sha256("ao-sdl:linux:" + machine_id);

    return crypto::sha256("ao-sdl:linux:unknown");
}

} // namespace platform

#include "platform/HardwareId.h"

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
// clang-format off
#include <windows.h>  // must precede bcrypt.h
#include <bcrypt.h>
#include <sddl.h>
// clang-format on

#include <iomanip>
#include <sstream>
#include <string>
#include <vector>

#pragma comment(lib, "bcrypt.lib")
#pragma comment(lib, "advapi32.lib")

static std::string sha256_hex(const std::string& input) {
    BCRYPT_ALG_HANDLE alg = nullptr;
    BCryptOpenAlgorithmProvider(&alg, BCRYPT_SHA256_ALGORITHM, nullptr, 0);
    if (!alg)
        return "";

    BCRYPT_HASH_HANDLE hash = nullptr;
    BCryptCreateHash(alg, &hash, nullptr, 0, nullptr, 0, 0);
    if (!hash) {
        BCryptCloseAlgorithmProvider(alg, 0);
        return "";
    }

    BCryptHashData(hash, (PUCHAR)input.data(), (ULONG)input.size(), 0);

    unsigned char digest[32];
    BCryptFinishHash(hash, digest, sizeof(digest), 0);
    BCryptDestroyHash(hash);
    BCryptCloseAlgorithmProvider(alg, 0);

    std::ostringstream ss;
    for (int i = 0; i < 32; i++)
        ss << std::hex << std::setfill('0') << std::setw(2) << (int)digest[i];
    return ss.str();
}

namespace platform {

std::string hardware_id() {
    // Primary: MachineGuid from the Windows registry.
    // This is a per-installation GUID that persists across reboots and user
    // changes. It's reset on OS reinstall, which is acceptable behavior.
    HKEY key = nullptr;
    if (RegOpenKeyExA(HKEY_LOCAL_MACHINE, "SOFTWARE\\Microsoft\\Cryptography", 0, KEY_READ | KEY_WOW64_64KEY, &key) ==
        ERROR_SUCCESS) {
        char buf[256] = {};
        DWORD size = sizeof(buf);
        DWORD type = 0;
        if (RegQueryValueExA(key, "MachineGuid", nullptr, &type, (LPBYTE)buf, &size) == ERROR_SUCCESS &&
            type == REG_SZ) {
            RegCloseKey(key);
            return sha256_hex(std::string("ao-sdl:win:") + buf);
        }
        RegCloseKey(key);
    }

    // Fallback: Windows SID of current user (same as AO2-Client)
    HANDLE token = nullptr;
    if (OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY, &token)) {
        DWORD len = 0;
        GetTokenInformation(token, TokenUser, nullptr, 0, &len);
        std::vector<uint8_t> buf(len);
        if (GetTokenInformation(token, TokenUser, buf.data(), len, &len)) {
            TOKEN_USER* user = (TOKEN_USER*)buf.data();
            LPSTR sid_str = nullptr;
            if (ConvertSidToStringSidA(user->User.Sid, &sid_str)) {
                std::string result = sha256_hex(std::string("ao-sdl:win:") + sid_str);
                LocalFree(sid_str);
                CloseHandle(token);
                return result;
            }
        }
        CloseHandle(token);
    }

    return sha256_hex("ao-sdl:win:unknown");
}

} // namespace platform

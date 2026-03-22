#include "platform/HardwareId.h"

#include <CommonCrypto/CommonDigest.h>
#import <Foundation/Foundation.h>
#import <IOKit/IOKitLib.h>

#include <iomanip>
#include <sstream>

static std::string sha256_hex(const std::string &input) {
    unsigned char hash[CC_SHA256_DIGEST_LENGTH];
    CC_SHA256(input.data(), (CC_LONG)input.size(), hash);

    std::ostringstream ss;
    for (int i = 0; i < CC_SHA256_DIGEST_LENGTH; i++)
        ss << std::hex << std::setfill('0') << std::setw(2) << (int)hash[i];
    return ss.str();
}

namespace platform {

std::string hardware_id() {
    @autoreleasepool {
        // Primary: IOPlatformUUID — a per-machine UUID that persists across
        // OS reinstalls (tied to hardware, not the OS installation).
        // This is more stable than the serial number and doesn't change
        // if the user replaces a drive.
        io_service_t platform =
            IOServiceGetMatchingService(kIOMainPortDefault, IOServiceMatching("IOPlatformExpertDevice"));

        if (platform) {
            CFStringRef uuid_cf = (CFStringRef)IORegistryEntryCreateCFProperty(platform, CFSTR(kIOPlatformUUIDKey),
                                                                               kCFAllocatorDefault, 0);
            IOObjectRelease(platform);

            if (uuid_cf) {
                char buf[128];
                if (CFStringGetCString(uuid_cf, buf, sizeof(buf), kCFStringEncodingUTF8)) {
                    CFRelease(uuid_cf);
                    return sha256_hex(std::string("ao-sdl:mac:") + buf);
                }
                CFRelease(uuid_cf);
            }
        }

        // Fallback: MAC address of en0 via IOKit
        // (less ideal — can change with hardware swaps)
        return sha256_hex("ao-sdl:mac:unknown");
    }
}

} // namespace platform

#pragma once
#include <cstdint>
namespace MiniCore {
struct Version {
    uint8_t major;
    uint8_t minor;
    uint8_t patch;
        [[nodiscard]] constexpr bool operator==(const Version& other) const {
        return major == other.major && minor == other.minor && patch == other.patch;
    }
        [[nodiscard]] constexpr bool operator<(const Version& other) const {
        if (major != other.major) return major < other.major;
        if (minor != other.minor) return minor < other.minor;
        return patch < other.patch;
    }
};
constexpr Version FIRMWARE_VERSION{0, 1, 0};
}

#pragma once

#include "core/config.h"
#include "core/result.h"
#include "hal/i_storage.h"
#include <cstddef>
#include <cstdint>

namespace MiniCore {

/**
 * Runtime preference: path hash mode (0, 1, or 2).
 * Matches MeshCore "path.hash.mode" CLI and prefs; used when sending
 * (path_hash_size = mode + 1). Stored in flash after identity and advert state.
 */
class PathHashMode {
public:
    PathHashMode() = default;

    /** Current value (0, 1, or 2). Default 0 until load() or set(). */
    [[nodiscard]] uint8_t get() const { return value_; }

    /** Set mode; valid values 0, 1, 2. Returns InvalidParameter if mode > 2. */
    Status set(uint8_t mode);

    /** Load from storage at given offset. Constrains stored value to 0..PATH_HASH_MODE_MAX. */
    Status load(IStorage& storage, size_t offset);

    /** Write current value to storage at offset; does not commit. */
    Status save(IStorage& storage, size_t offset) const;

private:
    uint8_t value_{Config::DEFAULT_PATH_HASH_MODE};
};

}  // namespace MiniCore

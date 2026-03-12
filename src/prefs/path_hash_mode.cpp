#include "prefs/path_hash_mode.h"
#include "core/constants.h"
#include <algorithm>

namespace MiniCore {

Status PathHashMode::set(uint8_t mode) {
    if (mode > Constants::PATH_HASH_MODE_MAX) {
        return ErrorCode::InvalidParameter;
    }
    value_ = mode;
    return Status();
}

Status PathHashMode::load(IStorage& storage, size_t offset) {
    auto res = storage.read(offset);
    if (res.isError()) {
        return res.error();
    }
    uint8_t stored = res.value();
    if (stored <= Constants::PATH_HASH_MODE_MAX) {
        value_ = stored;
    } else if (stored == 0xFF) {
        value_ = Config::DEFAULT_PATH_HASH_MODE;
    } else {
        value_ = Constants::PATH_HASH_MODE_MAX;
    }
    return Status();
}

Status PathHashMode::save(IStorage& storage, size_t offset) const {
    return storage.write(offset, value_);
}

}  // namespace MiniCore

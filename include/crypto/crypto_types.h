#pragma once
#include <cstdint>
#include <cstddef>
#include "core/config.h"
namespace MiniCore {
using Config::SEED_SIZE;
using Config::PUB_KEY_SIZE;
using Config::PRV_KEY_SIZE;
using Config::IDENTITY_STORAGE_SIZE;
struct PublicKey {
    uint8_t bytes[PUB_KEY_SIZE];
};
struct PrivateKey {
    uint8_t bytes[PRV_KEY_SIZE];
};
struct Seed {
    uint8_t bytes[SEED_SIZE];
};
struct LocalIdentity {
    PrivateKey privateKey;
    PublicKey publicKey;
        [[nodiscard]] bool isValid() const;
};
}

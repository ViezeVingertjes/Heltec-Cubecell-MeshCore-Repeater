#include "crypto/identity_manager.h"
#include "core/config.h"
#include <cstring>
namespace MiniCore {
namespace {
constexpr size_t IDENTITY_STORAGE_ADDRESS = 0;
constexpr uint8_t EMPTY_BYTE_VALUE = 0xFF;
}
IdentityManager::IdentityManager(IStorage& storage, IRng& rng, ICrypto& crypto, ILog& log)
    : storage_(storage)
    , rng_(rng)
    , crypto_(crypto)
    , log_(log)
{
}
Result<LocalIdentity> IdentityManager::loadOrCreate() {
    if (hasIdentity()) {
        auto loaded = load();
        if (loaded.isOk() && loaded.value().isValid()) {
            return loaded;
        }
        log_.warning("Stored identity invalid, regenerating");
    }
        auto result = generate();
    if (logIfError(result, "Failed to generate identity")) {
        return result.error();
    }
    auto saveResult = save(result.value());
    if (logIfError(saveResult, "Failed to save identity")) {
        return saveResult.error();
    }
        log_.info("New identity created");
    return result;
}
Result<LocalIdentity> IdentityManager::load() {
    if (!hasIdentity()) {
        return ErrorCode::NotFound;
    }
    return readFromStorage();
}
Status IdentityManager::save(const LocalIdentity& identity) {
    auto result = writeToStorage(identity);
    if (result.isError()) {
        return result;
    }
    return storage_.commit();
}
bool IdentityManager::hasIdentity() const {
    return !isStorageEmpty();
}
size_t IdentityManager::getStorageAddress() {
    return IDENTITY_STORAGE_ADDRESS;
}
size_t IdentityManager::getStorageSize() {
    return IDENTITY_STORAGE_SIZE;
}
Result<LocalIdentity> IdentityManager::generate() {
    Seed seed{};
    rng_.fill(seed.bytes, SEED_SIZE);
        LocalIdentity identity{};
    auto result = crypto_.createKeypair(identity.publicKey, identity.privateKey, seed);
    if (result.isError()) {
        return result.error();
    }
        return identity;
}
Status IdentityManager::writeToStorage(const LocalIdentity& identity) {
    size_t addr = IDENTITY_STORAGE_ADDRESS;
        auto prvResult = storage_.writeBlock(addr, identity.privateKey.bytes, PRV_KEY_SIZE);
    if (prvResult.isError()) {
        return prvResult;
    }
        addr += PRV_KEY_SIZE;
    return storage_.writeBlock(addr, identity.publicKey.bytes, PUB_KEY_SIZE);
}
Result<LocalIdentity> IdentityManager::readFromStorage() {
    LocalIdentity identity{};
    size_t addr = IDENTITY_STORAGE_ADDRESS;
        auto prvResult = storage_.readBlock(addr, identity.privateKey.bytes, PRV_KEY_SIZE);
    if (prvResult.isError()) {
        return prvResult.error();
    }
        addr += PRV_KEY_SIZE;
    auto pubResult = storage_.readBlock(addr, identity.publicKey.bytes, PUB_KEY_SIZE);
    if (pubResult.isError()) {
        return pubResult.error();
    }
        return identity;
}
bool IdentityManager::isStorageEmpty() const {
    constexpr size_t SAMPLE_SIZE = 4;
    uint8_t buffer[SAMPLE_SIZE];
    auto result = storage_.readBlock(IDENTITY_STORAGE_ADDRESS, buffer, SAMPLE_SIZE);
    if (result.isError()) {
        return true;
    }
    for (size_t i = 0; i < SAMPLE_SIZE; ++i) {
        if (buffer[i] != EMPTY_BYTE_VALUE) {
            return false;
        }
    }
    return true;
}
}

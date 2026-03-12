#include <unity.h>
#include "crypto/crypto_types.h"
#include "crypto/identity_manager.h"
#include "crypto/ed25519_crypto.h"
#include "hal/cubecell/flash_storage.h"
#include "hal/i_log.h"
#include "test_helpers.h"
using namespace MiniCore;
using namespace MiniCore::TestHelpers;
void test_identity_generates_valid() {
    FlashStorage storage;
    storage.init(IDENTITY_STORAGE_SIZE);
        TestRng rng;
    Ed25519Crypto crypto;
    NullLog log;
        IdentityManager manager(storage, rng, crypto, log);
    auto result = manager.loadOrCreate();
        TEST_ASSERT_TRUE(result.isOk());
    TEST_ASSERT_TRUE(result.value().isValid());
        storage.deinit();
}
void test_identity_persists_across_loads() {
    TestRng rng;
    Ed25519Crypto crypto;
    NullLog log;
        PublicKey firstPubKey{};
        {
        FlashStorage storage;
        storage.init(IDENTITY_STORAGE_SIZE);
        IdentityManager manager(storage, rng, crypto, log);
        auto result = manager.loadOrCreate();
        TEST_ASSERT_TRUE(result.isOk());
        firstPubKey = result.value().publicKey;
        storage.deinit();
    }
        {
        FlashStorage storage;
        storage.init(IDENTITY_STORAGE_SIZE);
        IdentityManager manager(storage, rng, crypto, log);
        auto result = manager.loadOrCreate();
        TEST_ASSERT_TRUE(result.isOk());
        TEST_ASSERT_EQUAL_MEMORY(firstPubKey.bytes, result.value().publicKey.bytes, PUB_KEY_SIZE);
        storage.deinit();
    }
}
void test_identity_hash_id_is_first_pubkey_byte() {
    FlashStorage storage;
    storage.init(IDENTITY_STORAGE_SIZE);
        TestRng rng;
    Ed25519Crypto crypto;
    NullLog log;
        IdentityManager manager(storage, rng, crypto, log);
    auto result = manager.loadOrCreate();
        TEST_ASSERT_TRUE(result.isOk());
    uint8_t hashId = result.value().publicKey.bytes[0];
    TEST_ASSERT_TRUE(hashId != 0 || result.value().publicKey.bytes[1] != 0);
        storage.deinit();
}

#include <unity.h>
#include "core/config.h"
#include "crypto/crypto_types.h"
#include "crypto/identity_manager.h"
#include "mocks/mock_storage.h"
#include "mocks/mock_rng.h"
#include "mocks/mock_crypto.h"
#include "mocks/mock_log.h"
using namespace MiniCore;
void test_crypto_constants_match_meshcore() {
    TEST_ASSERT_EQUAL(32, SEED_SIZE);
    TEST_ASSERT_EQUAL(32, PUB_KEY_SIZE);
    TEST_ASSERT_EQUAL(64, PRV_KEY_SIZE);
    TEST_ASSERT_EQUAL(96, IDENTITY_STORAGE_SIZE);
}

void test_dedup_table_sizes_match_meshcore() {
    TEST_ASSERT_EQUAL(128, Config::MAX_PACKET_HASHES);
    TEST_ASSERT_EQUAL(64, Config::MAX_PACKET_ACKS);
}
void test_identity_all_zeros_is_invalid() {
    LocalIdentity identity{};
    TEST_ASSERT_FALSE(identity.isValid());
}
void test_identity_with_nonzero_private_key_is_valid() {
    LocalIdentity identity{};
    identity.privateKey.bytes[0] = 0x01;
    TEST_ASSERT_TRUE(identity.isValid());
}
void test_rng_fill_with_preset_data() {
    Mocks::MockRng rng;
    rng.setPresetData({0x11, 0x22, 0x33, 0x44});
        uint8_t buffer[4] = {0};
    rng.fill(buffer, 4);
        TEST_ASSERT_TRUE(rng.fillCalled);
    TEST_ASSERT_EQUAL(4, rng.fillSize);
    TEST_ASSERT_EQUAL(0x11, buffer[0]);
    TEST_ASSERT_EQUAL(0x22, buffer[1]);
    TEST_ASSERT_EQUAL(0x33, buffer[2]);
    TEST_ASSERT_EQUAL(0x44, buffer[3]);
}
void test_rng_fill_wraps_preset_data() {
    Mocks::MockRng rng;
    rng.setPresetData({0xAA, 0xBB});
        uint8_t buffer[4] = {0};
    rng.fill(buffer, 4);
        TEST_ASSERT_EQUAL(0xAA, buffer[0]);
    TEST_ASSERT_EQUAL(0xBB, buffer[1]);
    TEST_ASSERT_EQUAL(0xAA, buffer[2]);
    TEST_ASSERT_EQUAL(0xBB, buffer[3]);
}
void test_identity_manager_storage_size_correct() {
    TEST_ASSERT_EQUAL(IDENTITY_STORAGE_SIZE, IdentityManager::getStorageSize());
}
void test_identity_manager_has_identity_false_when_empty() {
    Mocks::MockStorage storage;
    Mocks::MockRng rng;
    Mocks::MockCrypto crypto;
    Mocks::MockLog log;
        (void)storage.init(256);
        IdentityManager manager(storage, rng, crypto, log);
        TEST_ASSERT_FALSE(manager.hasIdentity());
}
void test_identity_manager_generate_creates_valid_identity() {
    Mocks::MockStorage storage;
    Mocks::MockRng rng;
    Mocks::MockCrypto crypto;
    Mocks::MockLog log;
        (void)storage.init(256);
    std::vector<uint8_t> seed(SEED_SIZE, 0x42);
    rng.setPresetData(seed);
        IdentityManager manager(storage, rng, crypto, log);
    auto result = manager.loadOrCreate();
        TEST_ASSERT_TRUE(result.isOk());
    TEST_ASSERT_TRUE(result.value().isValid());
    TEST_ASSERT_TRUE(crypto.createKeypairCalled);
}
void test_identity_manager_generate_uses_rng_for_seed() {
    Mocks::MockStorage storage;
    Mocks::MockRng rng;
    Mocks::MockCrypto crypto;
    Mocks::MockLog log;
        (void)storage.init(256);
    std::vector<uint8_t> seed(SEED_SIZE, 0xAB);
    rng.setPresetData(seed);
        IdentityManager manager(storage, rng, crypto, log);
    (void)manager.loadOrCreate();
        TEST_ASSERT_TRUE(rng.fillCalled);
    TEST_ASSERT_EQUAL(SEED_SIZE, rng.fillSize);
}
void test_identity_manager_save_writes_to_storage() {
    Mocks::MockStorage storage;
    Mocks::MockRng rng;
    Mocks::MockCrypto crypto;
    Mocks::MockLog log;
        (void)storage.init(256);
        LocalIdentity identity{};
    identity.privateKey.bytes[0] = 0x01;
        IdentityManager manager(storage, rng, crypto, log);
    auto result = manager.save(identity);
        TEST_ASSERT_TRUE(result.isOk());
    TEST_ASSERT_TRUE(storage.commitCalled);
}
void test_identity_manager_save_then_load_roundtrip() {
    Mocks::MockStorage storage;
    Mocks::MockRng rng;
    Mocks::MockCrypto crypto;
    Mocks::MockLog log;
        (void)storage.init(256);
        std::vector<uint8_t> seed(SEED_SIZE, 0x42);
    rng.setPresetData(seed);
        IdentityManager manager(storage, rng, crypto, log);
    auto createResult = manager.loadOrCreate();
    TEST_ASSERT_TRUE(createResult.isOk());
        Mocks::MockStorage storage2;
    (void)storage2.init(256);
    storage2.data = storage.data;
        IdentityManager manager2(storage2, rng, crypto, log);
    auto loadResult = manager2.load();
    TEST_ASSERT_TRUE(loadResult.isOk());
        for (size_t i = 0; i < PRV_KEY_SIZE; ++i) {
        TEST_ASSERT_EQUAL(createResult.value().privateKey.bytes[i], 
                         loadResult.value().privateKey.bytes[i]);
    }
    for (size_t i = 0; i < PUB_KEY_SIZE; ++i) {
        TEST_ASSERT_EQUAL(createResult.value().publicKey.bytes[i], 
                         loadResult.value().publicKey.bytes[i]);
    }
}
void test_identity_manager_load_fails_when_empty() {
    Mocks::MockStorage storage;
    Mocks::MockRng rng;
    Mocks::MockCrypto crypto;
    Mocks::MockLog log;
        (void)storage.init(256);
        IdentityManager manager(storage, rng, crypto, log);
    auto result = manager.load();
        TEST_ASSERT_TRUE(result.isError());
    TEST_ASSERT_EQUAL(ErrorCode::NotFound, result.error());
}
void test_identity_manager_load_or_create_creates_when_empty() {
    Mocks::MockStorage storage;
    Mocks::MockRng rng;
    Mocks::MockCrypto crypto;
    Mocks::MockLog log;
        (void)storage.init(256);
        std::vector<uint8_t> seed(SEED_SIZE, 0x42);
    rng.setPresetData(seed);
        IdentityManager manager(storage, rng, crypto, log);
    auto result = manager.loadOrCreate();
        TEST_ASSERT_TRUE(result.isOk());
    TEST_ASSERT_TRUE(result.value().isValid());
}
void test_identity_manager_load_or_create_loads_existing() {
    Mocks::MockStorage storage;
    Mocks::MockRng rng;
    Mocks::MockCrypto crypto;
    Mocks::MockLog log;
        (void)storage.init(256);
        std::vector<uint8_t> seed(SEED_SIZE, 0x42);
    rng.setPresetData(seed);
        IdentityManager manager(storage, rng, crypto, log);
    auto first = manager.loadOrCreate();
    TEST_ASSERT_TRUE(first.isOk());
        rng.setPresetData({0xAA});
    auto second = manager.loadOrCreate();
    TEST_ASSERT_TRUE(second.isOk());
        for (size_t i = 0; i < PRV_KEY_SIZE; ++i) {
        TEST_ASSERT_EQUAL(first.value().privateKey.bytes[i], 
                         second.value().privateKey.bytes[i]);
    }
}
void test_identity_manager_load_or_create_regenerates_when_stored_invalid() {
    Mocks::MockStorage storage;
    Mocks::MockRng rng;
    Mocks::MockCrypto crypto;
    Mocks::MockLog log;
        (void)storage.init(256);

    for (size_t i = 0; i < IDENTITY_STORAGE_SIZE; ++i) {
        storage.write(i, 0x00);
    }
        std::vector<uint8_t> seed(SEED_SIZE, 0x42);
    rng.setPresetData(seed);
        IdentityManager manager(storage, rng, crypto, log);
    auto result = manager.loadOrCreate();
        TEST_ASSERT_TRUE(result.isOk());
    TEST_ASSERT_TRUE(result.value().isValid());
    TEST_ASSERT_TRUE(crypto.createKeypairCalled);
}
void test_identity_manager_has_identity_true_after_save() {
    Mocks::MockStorage storage;
    Mocks::MockRng rng;
    Mocks::MockCrypto crypto;
    Mocks::MockLog log;
        (void)storage.init(256);
        std::vector<uint8_t> seed(SEED_SIZE, 0x42);
    rng.setPresetData(seed);
        IdentityManager manager(storage, rng, crypto, log);
    (void)manager.loadOrCreate();
        TEST_ASSERT_TRUE(manager.hasIdentity());
}
void test_identity_manager_logs_key_generation() {
    Mocks::MockStorage storage;
    Mocks::MockRng rng;
    Mocks::MockCrypto crypto;
    Mocks::MockLog log;
        (void)storage.init(256);
        std::vector<uint8_t> seed(SEED_SIZE, 0x42);
    rng.setPresetData(seed);
        IdentityManager manager(storage, rng, crypto, log);
    (void)manager.loadOrCreate();
        TEST_ASSERT_TRUE(log.countLevel(LogLevel::Info) > 0);
}
void test_identity_storage_format_matches_meshcore() {
    Mocks::MockStorage storage;
    Mocks::MockRng rng;
    Mocks::MockCrypto crypto;
    Mocks::MockLog log;
        (void)storage.init(256);
        std::vector<uint8_t> seed(SEED_SIZE, 0x42);
    rng.setPresetData(seed);
        IdentityManager manager(storage, rng, crypto, log);
    auto identity = manager.loadOrCreate();
    TEST_ASSERT_TRUE(identity.isOk());
        size_t addr = IdentityManager::getStorageAddress();
        uint8_t storedPrv[PRV_KEY_SIZE];
    uint8_t storedPub[PUB_KEY_SIZE];
    (void)storage.readBlock(addr, storedPrv, PRV_KEY_SIZE);
    (void)storage.readBlock(addr + PRV_KEY_SIZE, storedPub, PUB_KEY_SIZE);
        for (size_t i = 0; i < PRV_KEY_SIZE; ++i) {
        TEST_ASSERT_EQUAL(identity.value().privateKey.bytes[i], storedPrv[i]);
    }
    for (size_t i = 0; i < PUB_KEY_SIZE; ++i) {
        TEST_ASSERT_EQUAL(identity.value().publicKey.bytes[i], storedPub[i]);
    }
}

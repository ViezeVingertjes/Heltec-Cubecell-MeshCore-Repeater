#include <unity.h>
#include "core/config.h"
#include "prefs/path_hash_mode.h"
#include "mocks/mock_storage.h"
#include "hal/i_storage.h"

using namespace MiniCore;

void test_path_hash_mode_default_is_max() {
    PathHashMode prefs;
    TEST_ASSERT_EQUAL(Config::PATH_HASH_MODE_MAX, prefs.get());
}

void test_path_hash_mode_get_returns_stored_value() {
    PathHashMode prefs;
    prefs.set(1);
    TEST_ASSERT_EQUAL(1, prefs.get());
    prefs.set(2);
    TEST_ASSERT_EQUAL(2, prefs.get());
}

void test_path_hash_mode_set_accepts_0_1_2() {
    PathHashMode prefs;
    TEST_ASSERT_TRUE(prefs.set(0).isOk());
    TEST_ASSERT_EQUAL(0, prefs.get());
    TEST_ASSERT_TRUE(prefs.set(1).isOk());
    TEST_ASSERT_EQUAL(1, prefs.get());
    TEST_ASSERT_TRUE(prefs.set(2).isOk());
    TEST_ASSERT_EQUAL(2, prefs.get());
}

void test_path_hash_mode_set_rejects_3() {
    PathHashMode prefs;
    auto result = prefs.set(3);
    TEST_ASSERT_TRUE(result.isError());
    TEST_ASSERT_EQUAL(ErrorCode::InvalidParameter, result.error());
    TEST_ASSERT_EQUAL(Config::DEFAULT_PATH_HASH_MODE, prefs.get());
}

void test_path_hash_mode_set_rejects_larger_values() {
    PathHashMode prefs;
    prefs.set(1);
    TEST_ASSERT_TRUE(prefs.set(255).isError());
    TEST_ASSERT_EQUAL(1, prefs.get());
}

void test_path_hash_mode_load_from_storage() {
    Mocks::MockStorage storage;
    storage.init(Config::TOTAL_STORAGE_SIZE);
    uint8_t mode = 2;
    storage.writeBlock(Config::PATH_HASH_MODE_STORAGE_OFFSET, &mode, 1);

    PathHashMode prefs;
    auto st = prefs.load(storage, Config::PATH_HASH_MODE_STORAGE_OFFSET);
    TEST_ASSERT_TRUE(st.isOk());
    TEST_ASSERT_EQUAL(2, prefs.get());
}

void test_path_hash_mode_load_constrains_invalid_stored_value() {
    Mocks::MockStorage storage;
    storage.init(Config::TOTAL_STORAGE_SIZE);
    uint8_t invalid = 5;
    storage.writeBlock(Config::PATH_HASH_MODE_STORAGE_OFFSET, &invalid, 1);

    PathHashMode prefs;
    auto st = prefs.load(storage, Config::PATH_HASH_MODE_STORAGE_OFFSET);
    TEST_ASSERT_TRUE(st.isOk());
    TEST_ASSERT_EQUAL(Config::PATH_HASH_MODE_MAX, prefs.get());
}

void test_path_hash_mode_load_empty_storage_uses_default() {
    Mocks::MockStorage storage;
    storage.init(Config::TOTAL_STORAGE_SIZE);
    uint8_t empty = 0xFF;
    storage.writeBlock(Config::PATH_HASH_MODE_STORAGE_OFFSET, &empty, 1);

    PathHashMode prefs;
    auto st = prefs.load(storage, Config::PATH_HASH_MODE_STORAGE_OFFSET);
    TEST_ASSERT_TRUE(st.isOk());
    TEST_ASSERT_EQUAL(Config::DEFAULT_PATH_HASH_MODE, prefs.get());
}

void test_path_hash_mode_save_persists_value() {
    Mocks::MockStorage storage;
    storage.init(Config::TOTAL_STORAGE_SIZE);

    PathHashMode prefs;
    prefs.set(1);
    auto saveSt = prefs.save(storage, Config::PATH_HASH_MODE_STORAGE_OFFSET);
    TEST_ASSERT_TRUE(saveSt.isOk());

    PathHashMode prefs2;
    auto loadSt = prefs2.load(storage, Config::PATH_HASH_MODE_STORAGE_OFFSET);
    TEST_ASSERT_TRUE(loadSt.isOk());
    TEST_ASSERT_EQUAL(1, prefs2.get());
}

void test_path_hash_mode_save_then_load_roundtrip() {
    Mocks::MockStorage storage;
    storage.init(Config::TOTAL_STORAGE_SIZE);

    PathHashMode prefs;
    prefs.set(2);
    (void)prefs.save(storage, Config::PATH_HASH_MODE_STORAGE_OFFSET);

    PathHashMode loaded;
    (void)loaded.load(storage, Config::PATH_HASH_MODE_STORAGE_OFFSET);
    TEST_ASSERT_EQUAL(2, loaded.get());
}

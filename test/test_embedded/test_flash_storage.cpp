#include <unity.h>
#include "hal/cubecell/flash_storage.h"
using namespace MiniCore;
void test_flash_storage_init() {
    FlashStorage storage;
    auto result = storage.init(32);
    TEST_ASSERT_TRUE(result.isOk());
    TEST_ASSERT_EQUAL(32, storage.capacity());
    storage.deinit();
}
void test_flash_storage_write_read() {
    FlashStorage storage;
    storage.init(32);
        uint8_t testData[] = {0xDE, 0xAD, 0xBE, 0xEF};
    auto writeResult = storage.writeBlock(0, testData, 4);
    TEST_ASSERT_TRUE(writeResult.isOk());
        uint8_t readBack[4] = {0};
    auto readResult = storage.readBlock(0, readBack, 4);
    TEST_ASSERT_TRUE(readResult.isOk());
    TEST_ASSERT_EQUAL_MEMORY(testData, readBack, 4);
        storage.deinit();
}
void test_flash_storage_single_byte() {
    FlashStorage storage;
    storage.init(32);
        auto writeResult = storage.write(10, 0xAB);
    TEST_ASSERT_TRUE(writeResult.isOk());
        auto readResult = storage.read(10);
    TEST_ASSERT_TRUE(readResult.isOk());
    TEST_ASSERT_EQUAL(0xAB, readResult.value());
        storage.deinit();
}
void test_flash_storage_persists_after_commit() {
    uint8_t testData[] = {0xCA, 0xFE, 0xBA, 0xBE};
        {
        FlashStorage storage;
        storage.init(32);
        storage.writeBlock(8, testData, 4);
        storage.commit();
        storage.deinit();
    }
        {
        FlashStorage storage;
        storage.init(32);
        uint8_t readBack[4] = {0};
        storage.readBlock(8, readBack, 4);
        TEST_ASSERT_EQUAL_MEMORY(testData, readBack, 4);
        storage.deinit();
    }
}
void test_flash_storage_bounds_check() {
    FlashStorage storage;
    storage.init(16);
        auto result = storage.write(20, 0xFF);
    TEST_ASSERT_TRUE(result.isError());
    TEST_ASSERT_EQUAL(ErrorCode::InvalidParameter, result.error());
        storage.deinit();
}

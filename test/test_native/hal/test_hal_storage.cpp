#include <unity.h>
#include "mocks/mock_storage.h"
using namespace MiniCore;
void test_storage_init_sets_size() {
    Mocks::MockStorage storage;
        auto result = storage.init(256);
        TEST_ASSERT_TRUE(result.isOk());
    TEST_ASSERT_EQUAL(256, storage.capacity());
    TEST_ASSERT_TRUE(storage.initialized);
}
void test_storage_init_twice_fails() {
    Mocks::MockStorage storage;
    (void)storage.init(256);
        auto result = storage.init(128);
        TEST_ASSERT_TRUE(result.isError());
    TEST_ASSERT_EQUAL(ErrorCode::AlreadyInitialized, result.error());
}
void test_storage_read_write() {
    Mocks::MockStorage storage;
    (void)storage.init(256);
        (void)storage.write(10, 0x42);
    auto result = storage.read(10);
        TEST_ASSERT_TRUE(result.isOk());
    TEST_ASSERT_EQUAL(0x42, result.value());
}
void test_storage_read_uninitialized_fails() {
    Mocks::MockStorage storage;
        auto result = storage.read(0);
        TEST_ASSERT_TRUE(result.isError());
    TEST_ASSERT_EQUAL(ErrorCode::NotInitialized, result.error());
}
void test_storage_write_uninitialized_fails() {
    Mocks::MockStorage storage;
        auto result = storage.write(0, 0x42);
        TEST_ASSERT_TRUE(result.isError());
    TEST_ASSERT_EQUAL(ErrorCode::NotInitialized, result.error());
}
void test_storage_read_out_of_bounds_fails() {
    Mocks::MockStorage storage;
    (void)storage.init(16);
        auto result = storage.read(20);
        TEST_ASSERT_TRUE(result.isError());
    TEST_ASSERT_EQUAL(ErrorCode::InvalidParameter, result.error());
}
void test_storage_write_out_of_bounds_fails() {
    Mocks::MockStorage storage;
    (void)storage.init(16);
        auto result = storage.write(20, 0x42);
        TEST_ASSERT_TRUE(result.isError());
    TEST_ASSERT_EQUAL(ErrorCode::InvalidParameter, result.error());
}
void test_storage_block_read_write() {
    Mocks::MockStorage storage;
    (void)storage.init(256);
    uint8_t writeData[] = {0x01, 0x02, 0x03, 0x04};
    uint8_t readData[4] = {0};
        (void)storage.writeBlock(10, writeData, 4);
    auto result = storage.readBlock(10, readData, 4);
        TEST_ASSERT_TRUE(result.isOk());
    TEST_ASSERT_EQUAL(0x01, readData[0]);
    TEST_ASSERT_EQUAL(0x02, readData[1]);
    TEST_ASSERT_EQUAL(0x03, readData[2]);
    TEST_ASSERT_EQUAL(0x04, readData[3]);
}
void test_storage_commit_called() {
    Mocks::MockStorage storage;
    (void)storage.init(256);
        auto result = storage.commit();
        TEST_ASSERT_TRUE(result.isOk());
    TEST_ASSERT_TRUE(storage.commitCalled);
}
void test_storage_deinit() {
    Mocks::MockStorage storage;
    (void)storage.init(256);
        storage.deinit();
        TEST_ASSERT_FALSE(storage.initialized);
    TEST_ASSERT_EQUAL(0, storage.capacity());
}

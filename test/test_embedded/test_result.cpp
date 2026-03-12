#include <unity.h>
#include "core/result.h"
using namespace MiniCore;
void test_result_with_value() {
    Result<int> result(42);
    TEST_ASSERT_TRUE(result.isOk());
    TEST_ASSERT_EQUAL(42, result.value());
}
void test_result_with_error() {
    Result<int> result(ErrorCode::InvalidParameter);
    TEST_ASSERT_TRUE(result.isError());
    TEST_ASSERT_EQUAL(ErrorCode::InvalidParameter, result.error());
}
void test_status_ok() {
    Status status;
    TEST_ASSERT_TRUE(status.isOk());
}
void test_status_error() {
    Status status(ErrorCode::HardwareError);
    TEST_ASSERT_TRUE(status.isError());
}

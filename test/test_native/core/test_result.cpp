#include <unity.h>
#include "core/result.h"
using namespace MiniCore;
void test_result_with_value_is_ok() {
    Result<int> result(42);
    TEST_ASSERT_TRUE(result.isOk());
    TEST_ASSERT_FALSE(result.isError());
    TEST_ASSERT_EQUAL(42, result.value());
    TEST_ASSERT_EQUAL(ErrorCode::None, result.error());
}
void test_result_with_error_is_not_ok() {
    Result<int> result(ErrorCode::InvalidParameter);
    TEST_ASSERT_FALSE(result.isOk());
    TEST_ASSERT_TRUE(result.isError());
    TEST_ASSERT_EQUAL(ErrorCode::InvalidParameter, result.error());
}
void test_value_or_returns_value_when_ok() {
    Result<int> result(42);
    TEST_ASSERT_EQUAL(42, result.valueOr(0));
}
void test_value_or_returns_default_when_error() {
    Result<int> result(ErrorCode::Timeout);
    TEST_ASSERT_EQUAL(99, result.valueOr(99));
}
void test_status_success_is_ok() {
    Status status;
    TEST_ASSERT_TRUE(status.isOk());
    TEST_ASSERT_FALSE(status.isError());
    TEST_ASSERT_EQUAL(ErrorCode::None, status.error());
}
void test_status_with_error() {
    Status status(ErrorCode::HardwareError);
    TEST_ASSERT_FALSE(status.isOk());
    TEST_ASSERT_TRUE(status.isError());
    TEST_ASSERT_EQUAL(ErrorCode::HardwareError, status.error());
}
void test_all_error_codes() {
    TEST_ASSERT_EQUAL(0, static_cast<int>(ErrorCode::None));
    TEST_ASSERT_TRUE(Result<int>(ErrorCode::InvalidParameter).isError());
    TEST_ASSERT_TRUE(Result<int>(ErrorCode::BufferOverflow).isError());
    TEST_ASSERT_TRUE(Result<int>(ErrorCode::Timeout).isError());
    TEST_ASSERT_TRUE(Result<int>(ErrorCode::NotInitialized).isError());
    TEST_ASSERT_TRUE(Result<int>(ErrorCode::AlreadyInitialized).isError());
    TEST_ASSERT_TRUE(Result<int>(ErrorCode::HardwareError).isError());
    TEST_ASSERT_TRUE(Result<int>(ErrorCode::InvalidState).isError());
}

void test_map_preserves_value_when_ok() {
    Result<int> r(42);
    Result<int> mapped = r.map([](int x) { return x * 2; });
    TEST_ASSERT_TRUE(mapped.isOk());
    TEST_ASSERT_EQUAL(84, mapped.value());
}
void test_map_propagates_error() {
    Result<int> r(ErrorCode::InvalidParameter);
    Result<int> mapped = r.map([](int x) { return x * 2; });
    TEST_ASSERT_TRUE(mapped.isError());
    TEST_ASSERT_EQUAL(ErrorCode::InvalidParameter, mapped.error());
}
void test_map_changes_type() {
    Result<int> r(42);
    Result<unsigned> mapped = r.map([](int x) { return static_cast<unsigned>(x + 1); });
    TEST_ASSERT_TRUE(mapped.isOk());
    TEST_ASSERT_EQUAL(43u, mapped.value());
}

void test_and_then_continues_when_ok_returns_ok() {
    Result<int> r(42);
    Result<int> chained = r.and_then([](int x) { return Result<int>(x + 1); });
    TEST_ASSERT_TRUE(chained.isOk());
    TEST_ASSERT_EQUAL(43, chained.value());
}
void test_and_then_propagates_error_from_fn() {
    Result<int> r(42);
    Result<int> chained = r.and_then([](int) { return Result<int>(ErrorCode::Timeout); });
    TEST_ASSERT_TRUE(chained.isError());
    TEST_ASSERT_EQUAL(ErrorCode::Timeout, chained.error());
}
void test_and_then_propagates_initial_error() {
    Result<int> r(ErrorCode::BufferOverflow);
    Result<int> chained = r.and_then([](int x) { return Result<int>(x + 1); });
    TEST_ASSERT_TRUE(chained.isError());
    TEST_ASSERT_EQUAL(ErrorCode::BufferOverflow, chained.error());
}

void test_status_and_then_continues_when_ok_returns_ok() {
    Status s;
    Status chained = s.and_then([]() { return Status(); });
    TEST_ASSERT_TRUE(chained.isOk());
}
void test_status_and_then_propagates_error_from_fn() {
    Status s;
    Status chained = s.and_then([]() { return Status(ErrorCode::CryptoError); });
    TEST_ASSERT_TRUE(chained.isError());
    TEST_ASSERT_EQUAL(ErrorCode::CryptoError, chained.error());
}
void test_status_and_then_propagates_initial_error() {
    Status s(ErrorCode::NotFound);
    Status chained = s.and_then([]() { return Status(); });
    TEST_ASSERT_TRUE(chained.isError());
    TEST_ASSERT_EQUAL(ErrorCode::NotFound, chained.error());
}

void test_error_code_to_string_none() {
    const char* str = errorCodeToString(ErrorCode::None);
    TEST_ASSERT_NOT_NULL(str);
    TEST_ASSERT_EQUAL_STRING("None", str);
}
void test_error_code_to_string_invalid_parameter() {
    const char* str = errorCodeToString(ErrorCode::InvalidParameter);
    TEST_ASSERT_NOT_NULL(str);
    TEST_ASSERT_EQUAL_STRING("InvalidParameter", str);
}
void test_error_code_to_string_buffer_overflow() {
    const char* str = errorCodeToString(ErrorCode::BufferOverflow);
    TEST_ASSERT_NOT_NULL(str);
    TEST_ASSERT_EQUAL_STRING("BufferOverflow", str);
}
void test_error_code_to_string_timeout() {
    const char* str = errorCodeToString(ErrorCode::Timeout);
    TEST_ASSERT_NOT_NULL(str);
    TEST_ASSERT_EQUAL_STRING("Timeout", str);
}

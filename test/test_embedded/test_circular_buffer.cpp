#include <unity.h>
#include "util/circular_buffer.h"
using namespace MiniCore;
void test_buffer_starts_empty() {
    CircularBuffer<int, 8> buffer;
    TEST_ASSERT_TRUE(buffer.isEmpty());
    TEST_ASSERT_EQUAL(0, buffer.size());
}
void test_push_and_pop() {
    CircularBuffer<int, 8> buffer;
    TEST_ASSERT_TRUE(buffer.push(42).isOk());
    auto result = buffer.pop();
    TEST_ASSERT_TRUE(result.isOk());
    TEST_ASSERT_EQUAL(42, result.value());
}
void test_fifo_order() {
    CircularBuffer<int, 8> buffer;
    (void)buffer.push(1);
    (void)buffer.push(2);
    (void)buffer.push(3);
    TEST_ASSERT_EQUAL(1, buffer.pop().value());
    TEST_ASSERT_EQUAL(2, buffer.pop().value());
    TEST_ASSERT_EQUAL(3, buffer.pop().value());
}
void test_buffer_overflow_protection() {
    CircularBuffer<int, 2> buffer;
    TEST_ASSERT_TRUE(buffer.push(1).isOk());
    TEST_ASSERT_TRUE(buffer.push(2).isOk());
    TEST_ASSERT_TRUE(buffer.push(3).isError());
    TEST_ASSERT_EQUAL(ErrorCode::BufferOverflow, buffer.push(4).error());
}
void test_buffer_underflow_protection() {
    CircularBuffer<int, 2> buffer;
    TEST_ASSERT_TRUE(buffer.pop().isError());
    TEST_ASSERT_EQUAL(ErrorCode::InvalidState, buffer.pop().error());
}

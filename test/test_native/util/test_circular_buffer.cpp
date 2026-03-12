#include <unity.h>
#include "util/circular_buffer.h"
using namespace MiniCore;
void test_buffer_starts_empty() {
    CircularBuffer<int, 8> buffer;
    TEST_ASSERT_TRUE(buffer.isEmpty());
    TEST_ASSERT_FALSE(buffer.isFull());
    TEST_ASSERT_EQUAL(0, buffer.size());
    TEST_ASSERT_EQUAL(8, buffer.capacity());
}
void test_push_increases_size() {
    CircularBuffer<int, 8> buffer;
    auto result = buffer.push(42);
    TEST_ASSERT_TRUE(result.isOk());
    TEST_ASSERT_EQUAL(1, buffer.size());
    TEST_ASSERT_FALSE(buffer.isEmpty());
}
void test_pop_returns_pushed_value() {
    CircularBuffer<int, 8> buffer;
    (void)buffer.push(42);
    auto result = buffer.pop();
    TEST_ASSERT_TRUE(result.isOk());
    TEST_ASSERT_EQUAL(42, result.value());
    TEST_ASSERT_TRUE(buffer.isEmpty());
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
void test_push_fails_when_full() {
    CircularBuffer<int, 2> buffer;
    TEST_ASSERT_TRUE(buffer.push(1).isOk());
    TEST_ASSERT_TRUE(buffer.push(2).isOk());
    TEST_ASSERT_TRUE(buffer.isFull());
    auto result = buffer.push(3);
    TEST_ASSERT_TRUE(result.isError());
    TEST_ASSERT_EQUAL(ErrorCode::BufferOverflow, result.error());
}
void test_pop_fails_when_empty() {
    CircularBuffer<int, 8> buffer;
    auto result = buffer.pop();
    TEST_ASSERT_TRUE(result.isError());
    TEST_ASSERT_EQUAL(ErrorCode::InvalidState, result.error());
}
void test_peek_returns_value_without_removing() {
    CircularBuffer<int, 8> buffer;
    (void)buffer.push(42);
    auto peek_result = buffer.peek();
    TEST_ASSERT_TRUE(peek_result.isOk());
    TEST_ASSERT_EQUAL(42, peek_result.value());
    TEST_ASSERT_EQUAL(1, buffer.size());
}
void test_peek_fails_when_empty() {
    CircularBuffer<int, 8> buffer;
    auto result = buffer.peek();
    TEST_ASSERT_TRUE(result.isError());
    TEST_ASSERT_EQUAL(ErrorCode::InvalidState, result.error());
}
void test_clear_empties_buffer() {
    CircularBuffer<int, 8> buffer;
    (void)buffer.push(1);
    (void)buffer.push(2);
    buffer.clear();
    TEST_ASSERT_TRUE(buffer.isEmpty());
    TEST_ASSERT_EQUAL(0, buffer.size());
}
void test_wrap_around() {
    CircularBuffer<int, 4> buffer;
    (void)buffer.push(1);
    (void)buffer.push(2);
    (void)buffer.push(3);
    (void)buffer.pop();
    (void)buffer.pop();
    (void)buffer.push(4);
    (void)buffer.push(5);
    TEST_ASSERT_EQUAL(3, buffer.pop().value());
    TEST_ASSERT_EQUAL(4, buffer.pop().value());
    TEST_ASSERT_EQUAL(5, buffer.pop().value());
}
void test_buffer_fill_empty_cycle() {
    CircularBuffer<int, 3> buffer;
    for (int cycle = 0; cycle < 3; ++cycle) {
        TEST_ASSERT_TRUE(buffer.isEmpty());
        (void)buffer.push(1);
        (void)buffer.push(2);
        (void)buffer.push(3);
        TEST_ASSERT_TRUE(buffer.isFull());
        TEST_ASSERT_EQUAL(1, buffer.pop().value());
        TEST_ASSERT_EQUAL(2, buffer.pop().value());
        TEST_ASSERT_EQUAL(3, buffer.pop().value());
    }
}

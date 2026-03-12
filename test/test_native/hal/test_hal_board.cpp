#include <unity.h>
#include "mocks/mock_board.h"
using namespace MiniCore;
void test_board_reset() {
    Mocks::MockBoard board;
        board.reset();
        TEST_ASSERT_TRUE(board.resetCalled);
}
void test_board_get_unique_id() {
    Mocks::MockBoard board;
    board.deviceId = {{0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF, 0x00, 0x11}};
        auto id = board.getUniqueId();
        TEST_ASSERT_EQUAL(0xAA, id.bytes[0]);
    TEST_ASSERT_EQUAL(0xBB, id.bytes[1]);
    TEST_ASSERT_EQUAL(0x11, id.bytes[7]);
}
void test_board_get_random_seed() {
    Mocks::MockBoard board;
    board.randomSeed = 0xDEADBEEF;
        TEST_ASSERT_EQUAL(0xDEADBEEF, board.getRandomSeed());
}
void test_board_disable_enable_interrupts() {
    Mocks::MockBoard board;
        board.disableInterrupts();
    TEST_ASSERT_TRUE(board.interruptsDisabled);
        board.enableInterrupts();
    TEST_ASSERT_FALSE(board.interruptsDisabled);
}
void test_board_feed_watchdog() {
    Mocks::MockBoard board;
        board.feedWatchdog();
        TEST_ASSERT_TRUE(board.watchdogFed);
}

#include <unity.h>
#include "util/secure_wipe.h"
using namespace MiniCore;
void test_secure_wipe_zeros_memory() {
    uint8_t buf[32];
    for (size_t i = 0; i < sizeof(buf); ++i) {
        buf[i] = static_cast<uint8_t>(0xAAu + i);
    }
    secureWipe(buf, sizeof(buf));
    for (size_t i = 0; i < sizeof(buf); ++i) {
        TEST_ASSERT_EQUAL(0, buf[i]);
    }
}
void test_secure_wipe_zero_length_safe() {
    uint8_t buf[8] = {0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08};
    secureWipe(buf, 0);
    TEST_ASSERT_EQUAL(0x01, buf[0]);
    TEST_ASSERT_EQUAL(0x08, buf[7]);
}
void test_secure_wipe_partial() {
    uint8_t buf[8] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
    secureWipe(buf, 4);
    TEST_ASSERT_EQUAL(0, buf[0]);
    TEST_ASSERT_EQUAL(0, buf[3]);
    TEST_ASSERT_EQUAL(0xFF, buf[4]);
    TEST_ASSERT_EQUAL(0xFF, buf[7]);
}

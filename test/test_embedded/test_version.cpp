#include <unity.h>
#include "core/version.h"
using namespace MiniCore;
void test_version_defined() {
    TEST_ASSERT_EQUAL(0, FIRMWARE_VERSION.major);
    TEST_ASSERT_EQUAL(1, FIRMWARE_VERSION.minor);
    TEST_ASSERT_EQUAL(0, FIRMWARE_VERSION.patch);
}
void test_version_comparison() {
    Version v1{1, 0, 0};
    Version v2{2, 0, 0};
    TEST_ASSERT_TRUE(v1 < v2);
    TEST_ASSERT_FALSE(v2 < v1);
}

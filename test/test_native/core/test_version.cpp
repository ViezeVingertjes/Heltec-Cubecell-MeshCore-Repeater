#include <unity.h>
#include "core/version.h"
using namespace MiniCore;
void test_version_equality() {
    Version v1{1, 2, 3};
    Version v2{1, 2, 3};
    Version v3{1, 2, 4};
    TEST_ASSERT_TRUE(v1 == v2);
    TEST_ASSERT_FALSE(v1 == v3);
}
void test_version_comparison_major() {
    Version v1{1, 0, 0};
    Version v2{2, 0, 0};
    TEST_ASSERT_TRUE(v1 < v2);
    TEST_ASSERT_FALSE(v2 < v1);
}
void test_version_comparison_minor() {
    Version v1{1, 1, 0};
    Version v2{1, 2, 0};
    TEST_ASSERT_TRUE(v1 < v2);
    TEST_ASSERT_FALSE(v2 < v1);
}
void test_version_comparison_patch() {
    Version v1{1, 0, 1};
    Version v2{1, 0, 2};
    TEST_ASSERT_TRUE(v1 < v2);
    TEST_ASSERT_FALSE(v2 < v1);
}
void test_version_comparison_equal() {
    Version v1{1, 2, 3};
    Version v2{1, 2, 3};
    TEST_ASSERT_FALSE(v1 < v2);
    TEST_ASSERT_FALSE(v2 < v1);
}
void test_firmware_version_defined() {
    TEST_ASSERT_EQUAL(0, FIRMWARE_VERSION.major);
    TEST_ASSERT_EQUAL(1, FIRMWARE_VERSION.minor);
    TEST_ASSERT_EQUAL(0, FIRMWARE_VERSION.patch);
}

#include <unity.h>
#include "mocks/mock_power.h"
using namespace MiniCore;
void test_power_get_source() {
    Mocks::MockPower power;
    power.powerSource = PowerSource::Battery;
        TEST_ASSERT_EQUAL(PowerSource::Battery, power.getSource());
}
void test_power_get_battery_millivolts() {
    Mocks::MockPower power;
    power.batteryMv = 4200;
        TEST_ASSERT_EQUAL(4200, power.getBatteryMillivolts());
}
void test_power_get_battery_percent() {
    Mocks::MockPower power;
    power.batteryPercent = 100;
        TEST_ASSERT_EQUAL(100, power.getBatteryPercent());
}
void test_power_enter_sleep() {
    Mocks::MockPower power;
        power.enterSleep(SleepMode::DeepSleep);
        TEST_ASSERT_TRUE(power.enterSleepCalled);
    TEST_ASSERT_EQUAL(SleepMode::DeepSleep, power.currentSleepMode);
}
void test_power_wake_up() {
    Mocks::MockPower power;
    power.currentSleepMode = SleepMode::DeepSleep;
        power.wakeUp();
        TEST_ASSERT_TRUE(power.wakeUpCalled);
    TEST_ASSERT_EQUAL(SleepMode::Idle, power.currentSleepMode);
}
void test_power_enable_vext() {
    Mocks::MockPower power;
        power.enableVext(true);
    TEST_ASSERT_TRUE(power.vextEnabled);
        power.enableVext(false);
    TEST_ASSERT_FALSE(power.vextEnabled);
}

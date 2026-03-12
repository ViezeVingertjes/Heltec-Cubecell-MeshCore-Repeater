#include <unity.h>
#include "power/power_save.h"
#include "mocks/mock_radio.h"
#include "mocks/mock_timer.h"
using namespace MiniCore;

void test_power_save_initial_state_is_active() {
    Mocks::MockRadio radio;
    Mocks::MockTimer timer;
    PowerSaveController controller(radio, timer);
        TEST_ASSERT_EQUAL(PowerSaveState::Active, controller.state());
}
void test_power_save_initial_last_repeat_is_zero() {
    Mocks::MockRadio radio;
    Mocks::MockTimer timer;
    PowerSaveController controller(radio, timer);
        TEST_ASSERT_EQUAL(0, controller.lastRepeatTime());
}

void test_power_save_on_packet_repeated_updates_time() {
    Mocks::MockRadio radio;
    Mocks::MockTimer timer;
    PowerSaveController controller(radio, timer);
        timer.currentMillis = 5000;
    controller.onPacketRepeated();
        TEST_ASSERT_EQUAL(5000, controller.lastRepeatTime());
}
void test_power_save_multiple_repeats_update_time() {
    Mocks::MockRadio radio;
    Mocks::MockTimer timer;
    PowerSaveController controller(radio, timer);
        timer.currentMillis = 1000;
    controller.onPacketRepeated();
    TEST_ASSERT_EQUAL(1000, controller.lastRepeatTime());
        timer.currentMillis = 5000;
    controller.onPacketRepeated();
    TEST_ASSERT_EQUAL(5000, controller.lastRepeatTime());
}

void test_power_save_stays_active_before_timeout() {
    Mocks::MockRadio radio;
    Mocks::MockTimer timer;
    PowerSaveController controller(radio, timer);
        timer.currentMillis = 1000;
    controller.onPacketRepeated();

    timer.currentMillis = 1000 + Config::POWER_SAVE_IDLE_TIMEOUT_MS - 1;
    controller.process();
        TEST_ASSERT_EQUAL(PowerSaveState::Active, controller.state());
}
void test_power_save_enters_sleep_after_timeout() {
    Mocks::MockRadio radio;
    Mocks::MockTimer timer;
    PowerSaveController controller(radio, timer);
        timer.currentMillis = 1000;
    controller.onPacketRepeated();

    timer.currentMillis = 1000 + Config::POWER_SAVE_IDLE_TIMEOUT_MS;
    controller.process();
        TEST_ASSERT_EQUAL(PowerSaveState::Sleeping, controller.state());
}
void test_power_save_sleep_disables_rx() {
    Mocks::MockRadio radio;
    Mocks::MockTimer timer;
    PowerSaveController controller(radio, timer);
        timer.currentMillis = 1000;
    controller.onPacketRepeated();
        timer.currentMillis = 1000 + Config::POWER_SAVE_IDLE_TIMEOUT_MS;
    controller.process();
        TEST_ASSERT_TRUE(radio.sleepCalled);
}
void test_power_save_enters_sleep_with_no_repeats_from_start() {
    Mocks::MockRadio radio;
    Mocks::MockTimer timer;
    PowerSaveController controller(radio, timer);

    timer.currentMillis = Config::POWER_SAVE_IDLE_TIMEOUT_MS;
    controller.process();
        TEST_ASSERT_EQUAL(PowerSaveState::Sleeping, controller.state());
}

void test_power_save_wakes_after_sleep_duration() {
    Mocks::MockRadio radio;
    Mocks::MockTimer timer;
    PowerSaveController controller(radio, timer);

    timer.currentMillis = Config::POWER_SAVE_IDLE_TIMEOUT_MS;
    controller.process();
    TEST_ASSERT_EQUAL(PowerSaveState::Sleeping, controller.state());

    timer.currentMillis = Config::POWER_SAVE_IDLE_TIMEOUT_MS + Config::POWER_SAVE_SLEEP_DURATION_MS;
    controller.process();
        TEST_ASSERT_EQUAL(PowerSaveState::Active, controller.state());
}
void test_power_save_wake_enables_rx() {
    Mocks::MockRadio radio;
    Mocks::MockTimer timer;
    PowerSaveController controller(radio, timer);

    timer.currentMillis = Config::POWER_SAVE_IDLE_TIMEOUT_MS;
    controller.process();
        radio.receiveCalled = false;

    timer.currentMillis = Config::POWER_SAVE_IDLE_TIMEOUT_MS + Config::POWER_SAVE_SLEEP_DURATION_MS;
    controller.process();
        TEST_ASSERT_TRUE(radio.receiveCalled);
}
void test_power_save_stays_sleeping_before_duration() {
    Mocks::MockRadio radio;
    Mocks::MockTimer timer;
    PowerSaveController controller(radio, timer);

    timer.currentMillis = Config::POWER_SAVE_IDLE_TIMEOUT_MS;
    controller.process();

    timer.currentMillis = Config::POWER_SAVE_IDLE_TIMEOUT_MS + Config::POWER_SAVE_SLEEP_DURATION_MS - 1;
    controller.process();
        TEST_ASSERT_EQUAL(PowerSaveState::Sleeping, controller.state());
}

void test_power_save_repeat_resets_timeout() {
    Mocks::MockRadio radio;
    Mocks::MockTimer timer;
    PowerSaveController controller(radio, timer);
        timer.currentMillis = 1000;
    controller.onPacketRepeated();

    timer.currentMillis = 1000 + Config::POWER_SAVE_IDLE_TIMEOUT_MS - 100;
    controller.process();
    TEST_ASSERT_EQUAL(PowerSaveState::Active, controller.state());

    controller.onPacketRepeated();

    timer.currentMillis = 1000 + Config::POWER_SAVE_IDLE_TIMEOUT_MS + 100;
    controller.process();
    TEST_ASSERT_EQUAL(PowerSaveState::Active, controller.state());

    timer.currentMillis = (1000 + Config::POWER_SAVE_IDLE_TIMEOUT_MS - 100) + Config::POWER_SAVE_IDLE_TIMEOUT_MS;
    controller.process();
    TEST_ASSERT_EQUAL(PowerSaveState::Sleeping, controller.state());
}
void test_power_save_wake_resets_last_repeat_time() {
    Mocks::MockRadio radio;
    Mocks::MockTimer timer;
    PowerSaveController controller(radio, timer);

    timer.currentMillis = Config::POWER_SAVE_IDLE_TIMEOUT_MS;
    controller.process();

    uint32_t wakeTime = Config::POWER_SAVE_IDLE_TIMEOUT_MS + Config::POWER_SAVE_SLEEP_DURATION_MS;
    timer.currentMillis = wakeTime;
    controller.process();

    TEST_ASSERT_EQUAL(wakeTime, controller.lastRepeatTime());
}

void test_power_save_full_cycle_sleep_wake_sleep() {
    Mocks::MockRadio radio;
    Mocks::MockTimer timer;
    PowerSaveController controller(radio, timer);

    timer.currentMillis = Config::POWER_SAVE_IDLE_TIMEOUT_MS;
    controller.process();
    TEST_ASSERT_EQUAL(PowerSaveState::Sleeping, controller.state());

    timer.currentMillis += Config::POWER_SAVE_SLEEP_DURATION_MS;
    controller.process();
    TEST_ASSERT_EQUAL(PowerSaveState::Active, controller.state());

    timer.currentMillis += Config::POWER_SAVE_IDLE_TIMEOUT_MS;
    controller.process();
    TEST_ASSERT_EQUAL(PowerSaveState::Sleeping, controller.state());

    timer.currentMillis += Config::POWER_SAVE_SLEEP_DURATION_MS;
    controller.process();
    TEST_ASSERT_EQUAL(PowerSaveState::Active, controller.state());
}
void test_power_save_repeat_during_active_prevents_sleep() {
    Mocks::MockRadio radio;
    Mocks::MockTimer timer;
    PowerSaveController controller(radio, timer);

    for (int i = 0; i < 5; ++i) {
        timer.currentMillis = i * 30000;
        controller.onPacketRepeated();
        controller.process();
        TEST_ASSERT_EQUAL(PowerSaveState::Active, controller.state());
    }
}

void test_power_save_enabled_disabled() {
    Mocks::MockRadio radio;
    Mocks::MockTimer timer;
    PowerSaveController controller(radio, timer);
        controller.setEnabled(false);

    timer.currentMillis = Config::POWER_SAVE_IDLE_TIMEOUT_MS * 2;
    controller.process();
        TEST_ASSERT_EQUAL(PowerSaveState::Active, controller.state());
    TEST_ASSERT_FALSE(radio.sleepCalled);
}
void test_power_save_is_sleeping_helper() {
    Mocks::MockRadio radio;
    Mocks::MockTimer timer;
    PowerSaveController controller(radio, timer);
        TEST_ASSERT_FALSE(controller.isSleeping());
        timer.currentMillis = Config::POWER_SAVE_IDLE_TIMEOUT_MS;
    controller.process();
        TEST_ASSERT_TRUE(controller.isSleeping());
}
void test_power_save_custom_config() {
    Mocks::MockRadio radio;
    Mocks::MockTimer timer;
    PowerSaveConfig config;
    config.idleTimeoutMs = 10000;
    config.sleepDurationMs = 5000;
    PowerSaveController controller(radio, timer, config);

    timer.currentMillis = 10000;
    controller.process();
    TEST_ASSERT_EQUAL(PowerSaveState::Sleeping, controller.state());

    timer.currentMillis = 15000;
    controller.process();
    TEST_ASSERT_EQUAL(PowerSaveState::Active, controller.state());
}
void test_power_save_config_defaults() {
    PowerSaveConfig config = PowerSaveConfig::defaults();
        TEST_ASSERT_EQUAL(Config::POWER_SAVE_IDLE_TIMEOUT_MS, config.idleTimeoutMs);
    TEST_ASSERT_EQUAL(Config::POWER_SAVE_SLEEP_DURATION_MS, config.sleepDurationMs);
}

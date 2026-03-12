#include <unity.h>
#include "radio/radio_controller.h"
#include "mocks/mock_radio.h"
#include "mocks/mock_timer.h"
using namespace MiniCore;
class TestRadioControllerEvents : public IRadioControllerEvents {
public:
    bool transmitCompleteCalled{false};
    bool lastTransmitSuccess{false};
    bool channelBusyCalled{false};
    uint32_t transmitCompleteCount{0};
    uint32_t channelBusyCount{0};
        void onTransmitComplete(bool success) override {
        transmitCompleteCalled = true;
        lastTransmitSuccess = success;
        ++transmitCompleteCount;
    }
        void onChannelBusy() override {
        channelBusyCalled = true;
        ++channelBusyCount;
    }
        void reset() {
        transmitCompleteCalled = false;
        lastTransmitSuccess = false;
        channelBusyCalled = false;
        transmitCompleteCount = 0;
        channelBusyCount = 0;
    }
};

void test_radio_controller_initial_state_is_idle() {
    Mocks::MockRadio radio;
    Mocks::MockTimer timer;
    RadioController controller(radio, timer);
        TEST_ASSERT_EQUAL(RadioControllerState::Idle, controller.state());
    TEST_ASSERT_FALSE(controller.isBusy());
}
void test_radio_controller_transmits_immediately_when_cs_clear() {
    Mocks::MockRadio radio;
    Mocks::MockTimer timer;
    RadioController controller(radio, timer);
    radio.setEventHandler(&controller);

    uint8_t data[] = {0x01, 0x02, 0x03};
    auto result = controller.transmitWithCS(data, 3);
        TEST_ASSERT_TRUE(result.isOk());
    TEST_ASSERT_EQUAL(RadioControllerState::Transmitting, controller.state());
    TEST_ASSERT_TRUE(controller.isBusy());
    TEST_ASSERT_EQUAL(3, radio.lastSentData.size());
}
void test_radio_controller_completes_after_tx_done() {
    Mocks::MockRadio radio;
    Mocks::MockTimer timer;
    TestRadioControllerEvents events;
    RadioController controller(radio, timer);
    radio.setEventHandler(&controller);
    controller.setEventHandler(&events);
        uint8_t data[] = {0x01, 0x02, 0x03};
    controller.transmitWithCS(data, 3);
    radio.simulateTxDone();
        TEST_ASSERT_EQUAL(RadioControllerState::Idle, controller.state());
    TEST_ASSERT_TRUE(events.transmitCompleteCalled);
    TEST_ASSERT_TRUE(events.lastTransmitSuccess);
}

void test_radio_controller_schedules_retry_on_carrier_detected() {
    Mocks::MockRadio radio;
    Mocks::MockTimer timer;
    TestRadioControllerEvents events;
    RadioController controller(radio, timer);
    radio.setEventHandler(&controller);
    controller.setEventHandler(&events);
    controller.enableCarrierSense(true);

    radio.instantRssi = -100;
    for (int i = 0; i < 20; ++i) {
        controller.updateNoiseFloor();
    }

    radio.instantRssi = -70;
        uint8_t data[] = {0x01, 0x02, 0x03};
    controller.transmitWithCS(data, 3);
        TEST_ASSERT_EQUAL(RadioControllerState::AwaitingRetry, controller.state());
    TEST_ASSERT_EQUAL(1, controller.retryCount());
    TEST_ASSERT_TRUE(events.channelBusyCalled);
}
void test_radio_controller_retries_cs_after_delay() {
    Mocks::MockRadio radio;
    Mocks::MockTimer timer;
    TestRadioControllerEvents events;
    CarrierSenseConfig config = CarrierSenseConfig::defaults();
    config.retryDelayMs = 100;
    RadioController controller(radio, timer, config);
    radio.setEventHandler(&controller);
    controller.setEventHandler(&events);
    controller.enableCarrierSense(true);

    radio.instantRssi = -100;
    for (int i = 0; i < 20; ++i) {
        controller.updateNoiseFloor();
    }

    radio.instantRssi = -70;
    timer.currentMillis = 1000;
        uint8_t data[] = {0x01, 0x02, 0x03};
    controller.transmitWithCS(data, 3);
        TEST_ASSERT_EQUAL(RadioControllerState::AwaitingRetry, controller.state());

    timer.advance(50);
    controller.process(timer.millis());
    TEST_ASSERT_EQUAL(RadioControllerState::AwaitingRetry, controller.state());

    radio.instantRssi = -100;

    timer.advance(60);
    controller.process(timer.millis());

    TEST_ASSERT_EQUAL(RadioControllerState::Transmitting, controller.state());
}
void test_radio_controller_transmits_on_retry_success() {
    Mocks::MockRadio radio;
    Mocks::MockTimer timer;
    TestRadioControllerEvents events;
    CarrierSenseConfig config = CarrierSenseConfig::defaults();
    config.retryDelayMs = 100;
    RadioController controller(radio, timer, config);
    radio.setEventHandler(&controller);
    controller.setEventHandler(&events);
    controller.enableCarrierSense(true);

    radio.instantRssi = -100;
    for (int i = 0; i < 20; ++i) {
        controller.updateNoiseFloor();
    }

    radio.instantRssi = -70;
    timer.currentMillis = 1000;
    uint8_t data[] = {0xAA, 0xBB};
    controller.transmitWithCS(data, 2);
        TEST_ASSERT_EQUAL(RadioControllerState::AwaitingRetry, controller.state());

    radio.instantRssi = -100;
        timer.advance(150);
    controller.process(timer.millis());
        TEST_ASSERT_EQUAL(RadioControllerState::Transmitting, controller.state());
    TEST_ASSERT_EQUAL(2, radio.lastSentData.size());
    TEST_ASSERT_EQUAL(0xAA, radio.lastSentData[0]);
}
void test_radio_controller_transmits_after_max_retries() {
    Mocks::MockRadio radio;
    Mocks::MockTimer timer;
    TestRadioControllerEvents events;
    CarrierSenseConfig config;
    config.maxRetries = 3;
    config.retryDelayMs = 50;
    RadioController controller(radio, timer, config);
    radio.setEventHandler(&controller);
    controller.setEventHandler(&events);
    controller.enableCarrierSense(true);

    radio.instantRssi = -100;
    for (int i = 0; i < 20; ++i) {
        controller.updateNoiseFloor();
    }

    radio.instantRssi = -70;
    timer.currentMillis = 1000;
        uint8_t data[] = {0x01};
    controller.transmitWithCS(data, 1);

    for (uint8_t i = 0; i < config.maxRetries; ++i) {
        TEST_ASSERT_EQUAL(RadioControllerState::AwaitingRetry, controller.state());
        timer.advance(100);
        controller.process(timer.millis());
    }

    TEST_ASSERT_EQUAL(RadioControllerState::Transmitting, controller.state());
    TEST_ASSERT_EQUAL(1, radio.lastSentData.size());
    TEST_ASSERT_EQUAL(0x01, radio.lastSentData[0]);
    TEST_ASSERT_EQUAL(config.maxRetries + 1, events.channelBusyCount);

    radio.simulateTxDone();
    TEST_ASSERT_EQUAL(RadioControllerState::Idle, controller.state());
    TEST_ASSERT_TRUE(events.transmitCompleteCalled);
    TEST_ASSERT_TRUE(events.lastTransmitSuccess);
}

void test_radio_controller_rejects_transmit_when_busy() {
    Mocks::MockRadio radio;
    Mocks::MockTimer timer;
    RadioController controller(radio, timer);
    radio.setEventHandler(&controller);
        uint8_t data[] = {0x01};
    controller.transmitWithCS(data, 1);
        auto result = controller.transmitWithCS(data, 1);
        TEST_ASSERT_FALSE(result.isOk());
    TEST_ASSERT_EQUAL(ErrorCode::Busy, result.error());
}
void test_radio_controller_handles_tx_timeout() {
    Mocks::MockRadio radio;
    Mocks::MockTimer timer;
    TestRadioControllerEvents events;
    RadioController controller(radio, timer);
    radio.setEventHandler(&controller);
    controller.setEventHandler(&events);
        uint8_t data[] = {0x01};
    controller.transmitWithCS(data, 1);
    radio.simulateTxTimeout();
        TEST_ASSERT_EQUAL(RadioControllerState::Idle, controller.state());
    TEST_ASSERT_TRUE(events.transmitCompleteCalled);
    TEST_ASSERT_FALSE(events.lastTransmitSuccess);
}
void test_radio_controller_rejects_oversized_data() {
    Mocks::MockRadio radio;
    Mocks::MockTimer timer;
    RadioController controller(radio, timer);
        uint8_t data[300];
    auto result = controller.transmitWithCS(data, 255);

    TEST_ASSERT_TRUE(result.isOk() || result.error() == ErrorCode::BufferTooSmall);
}

void test_radio_controller_uses_custom_cs_config() {
    Mocks::MockRadio radio;
    Mocks::MockTimer timer;
    CarrierSenseConfig config;
    config.maxRetries = 5;
    config.retryDelayMs = 200;
    RadioController controller(radio, timer, config);
    radio.setEventHandler(&controller);
    controller.enableCarrierSense(true);

    radio.instantRssi = -100;
    for (int i = 0; i < 20; ++i) {
        controller.updateNoiseFloor();
    }

    radio.instantRssi = -70;
        uint8_t data[] = {0x01};
    controller.transmitWithCS(data, 1);
        TEST_ASSERT_EQUAL(RadioControllerState::AwaitingRetry, controller.state());
}
void test_cs_config_defaults() {
    CarrierSenseConfig config = CarrierSenseConfig::defaults();
        TEST_ASSERT_EQUAL(Config::CS_MAX_RETRIES, config.maxRetries);
    TEST_ASSERT_EQUAL(Config::CS_RETRY_DELAY_MS, config.retryDelayMs);
}

void test_radio_controller_does_not_affect_rx_during_idle() {
    Mocks::MockRadio radio;
    Mocks::MockTimer timer;
    RadioController controller(radio, timer);
    radio.setEventHandler(&controller);

    uint8_t rxData[] = {0x01, 0x02};
    RxPacket pkt{rxData, 2, -50, 10};
    controller.onRxDone(pkt);
    controller.onRxTimeout();
    controller.onRxError();
        TEST_ASSERT_EQUAL(RadioControllerState::Idle, controller.state());
}

void test_radio_controller_uses_source_pointer_directly() {
    Mocks::MockRadio radio;
    Mocks::MockTimer timer;
    RadioController controller(radio, timer);
    radio.setEventHandler(&controller);
        uint8_t data[] = {0xAA, 0xBB, 0xCC};
    controller.transmitWithCS(data, 3);
        TEST_ASSERT_EQUAL(3, radio.lastSentData.size());
    TEST_ASSERT_EQUAL(0xAA, radio.lastSentData[0]);
    TEST_ASSERT_EQUAL(0xBB, radio.lastSentData[1]);
    TEST_ASSERT_EQUAL(0xCC, radio.lastSentData[2]);
}
void test_radio_controller_data_valid_through_retries() {
    Mocks::MockRadio radio;
    Mocks::MockTimer timer;
    CarrierSenseConfig config = CarrierSenseConfig::defaults();
    config.retryDelayMs = 100;
    RadioController controller(radio, timer, config);
    radio.setEventHandler(&controller);
    controller.enableCarrierSense(true);

    radio.instantRssi = -100;
    for (int i = 0; i < 20; ++i) {
        controller.updateNoiseFloor();
    }
        uint8_t data[] = {0x11, 0x22, 0x33, 0x44};
    timer.currentMillis = 1000;

    radio.instantRssi = -70;
    controller.transmitWithCS(data, 4);

    radio.instantRssi = -100;
        timer.advance(150);
    controller.process(timer.millis());
        TEST_ASSERT_EQUAL(4, radio.lastSentData.size());
    TEST_ASSERT_EQUAL(0x11, radio.lastSentData[0]);
    TEST_ASSERT_EQUAL(0x44, radio.lastSentData[3]);
}
void test_radio_controller_reports_pending_data_size() {
    Mocks::MockRadio radio;
    Mocks::MockTimer timer;
    RadioController controller(radio, timer);
    radio.setEventHandler(&controller);
        TEST_ASSERT_EQUAL(0, controller.pendingSize());
        uint8_t data[] = {0x01, 0x02, 0x03, 0x04, 0x05};
    controller.transmitWithCS(data, 5);
        TEST_ASSERT_EQUAL(5, controller.pendingSize());
        radio.simulateTxDone();
        TEST_ASSERT_EQUAL(0, controller.pendingSize());
}

void test_radio_controller_carrier_sense_disabled_by_default() {
    Mocks::MockRadio radio;
    Mocks::MockTimer timer;
    RadioController controller(radio, timer);
        TEST_ASSERT_FALSE(controller.isCarrierSenseEnabled());
}
void test_radio_controller_enable_carrier_sense() {
    Mocks::MockRadio radio;
    Mocks::MockTimer timer;
    RadioController controller(radio, timer);
        controller.enableCarrierSense(true);
        TEST_ASSERT_TRUE(controller.isCarrierSenseEnabled());
}
void test_radio_controller_carrier_sense_samples_noise_floor() {
    Mocks::MockRadio radio;
    Mocks::MockTimer timer;
    RadioController controller(radio, timer);
    controller.enableCarrierSense(true);

    radio.instantRssi = -110;
    for (int i = 0; i < 20; ++i) {
        controller.updateNoiseFloor();
    }
        TEST_ASSERT_TRUE(controller.isNoiseFloorValid());
    TEST_ASSERT_INT16_WITHIN(5, -110, controller.getNoiseFloor());
}
void test_radio_controller_carrier_sense_detects_signal_above_threshold() {
    Mocks::MockRadio radio;
    Mocks::MockTimer timer;
    RadioController controller(radio, timer);
    controller.enableCarrierSense(true);

    radio.instantRssi = -100;
    for (int i = 0; i < 20; ++i) {
        controller.updateNoiseFloor();
    }

    radio.instantRssi = -80;
    TEST_ASSERT_TRUE(controller.isCarrierDetected());
}
void test_radio_controller_carrier_sense_ignores_noise() {
    Mocks::MockRadio radio;
    Mocks::MockTimer timer;
    RadioController controller(radio, timer);
    controller.enableCarrierSense(true);

    radio.instantRssi = -100;
    for (int i = 0; i < 20; ++i) {
        controller.updateNoiseFloor();
    }

    radio.instantRssi = -100;
    TEST_ASSERT_FALSE(controller.isCarrierDetected());
}
void test_radio_controller_carrier_sense_schedules_retry_on_carrier() {
    Mocks::MockRadio radio;
    Mocks::MockTimer timer;
    TestRadioControllerEvents events;
    RadioController controller(radio, timer);
    radio.setEventHandler(&controller);
    controller.setEventHandler(&events);
    controller.enableCarrierSense(true);

    radio.instantRssi = -100;
    for (int i = 0; i < 20; ++i) {
        controller.updateNoiseFloor();
    }

    radio.instantRssi = -70;
        uint8_t data[] = {0x01, 0x02, 0x03};
    timer.currentMillis = 1000;
    controller.transmitWithCS(data, 3);

    TEST_ASSERT_EQUAL(RadioControllerState::AwaitingRetry, controller.state());
    TEST_ASSERT_TRUE(events.channelBusyCalled);
}
void test_radio_controller_carrier_sense_transmits_when_clear() {
    Mocks::MockRadio radio;
    Mocks::MockTimer timer;
    RadioController controller(radio, timer);
    radio.setEventHandler(&controller);
    controller.enableCarrierSense(true);

    radio.instantRssi = -100;
    for (int i = 0; i < 20; ++i) {
        controller.updateNoiseFloor();
    }

    radio.instantRssi = -100;
        uint8_t data[] = {0x01, 0x02, 0x03};
    controller.transmitWithCS(data, 3);

    TEST_ASSERT_EQUAL(RadioControllerState::Transmitting, controller.state());
    TEST_ASSERT_EQUAL(3, radio.lastSentData.size());
}
void test_radio_controller_carrier_sense_disabled_transmits_immediately() {
    Mocks::MockRadio radio;
    Mocks::MockTimer timer;
    RadioController controller(radio, timer);
    radio.setEventHandler(&controller);

    radio.instantRssi = -50;
        uint8_t data[] = {0x01, 0x02, 0x03};
    controller.transmitWithCS(data, 3);
        TEST_ASSERT_EQUAL(RadioControllerState::Transmitting, controller.state());
}
void test_radio_controller_carrier_sense_invalid_noise_floor_transmits() {
    Mocks::MockRadio radio;
    Mocks::MockTimer timer;
    RadioController controller(radio, timer);
    radio.setEventHandler(&controller);
    controller.enableCarrierSense(true);

    TEST_ASSERT_FALSE(controller.isNoiseFloorValid());

    radio.instantRssi = -50;
        uint8_t data[] = {0x01, 0x02, 0x03};
    controller.transmitWithCS(data, 3);
        TEST_ASSERT_EQUAL(RadioControllerState::Transmitting, controller.state());
}
void test_radio_controller_carrier_sense_retry_rechecks_rssi() {
    Mocks::MockRadio radio;
    Mocks::MockTimer timer;
    TestRadioControllerEvents events;
    CarrierSenseConfig config = CarrierSenseConfig::defaults();
    config.retryDelayMs = 100;
    RadioController controller(radio, timer, config);
    radio.setEventHandler(&controller);
    controller.setEventHandler(&events);
    controller.enableCarrierSense(true);

    radio.instantRssi = -100;
    for (int i = 0; i < 20; ++i) {
        controller.updateNoiseFloor();
    }

    radio.instantRssi = -70;
    timer.currentMillis = 1000;
    uint8_t data[] = {0x01};
    controller.transmitWithCS(data, 1);
        TEST_ASSERT_EQUAL(RadioControllerState::AwaitingRetry, controller.state());

    timer.advance(150);
    controller.process(timer.millis());

    TEST_ASSERT_EQUAL(RadioControllerState::AwaitingRetry, controller.state());
    TEST_ASSERT_EQUAL(2, events.channelBusyCount);

    radio.instantRssi = -100;
    timer.advance(150);
    controller.process(timer.millis());

    TEST_ASSERT_EQUAL(RadioControllerState::Transmitting, controller.state());
}
void test_radio_controller_configure_carrier_sense_for_bandwidth() {
    Mocks::MockRadio radio;
    Mocks::MockTimer timer;
    RadioController controller(radio, timer);

    controller.configureCarrierSense(LoraBandwidth::BW_500);
        TEST_ASSERT_TRUE(controller.isCarrierSenseEnabled());

    radio.instantRssi = -100;
    for (int i = 0; i < 20; ++i) {
        controller.updateNoiseFloor();
    }

    radio.instantRssi = -90;
    TEST_ASSERT_FALSE(controller.isCarrierDetected());

    radio.instantRssi = -80;
    TEST_ASSERT_TRUE(controller.isCarrierDetected());
}
void test_radio_controller_narrow_bandwidth_uses_smaller_margin() {
    Mocks::MockRadio radio;
    Mocks::MockTimer timer;
    RadioController controller(radio, timer);

    controller.configureCarrierSense(LoraBandwidth::BW_7_8);

    radio.instantRssi = -100;
    for (int i = 0; i < 20; ++i) {
        controller.updateNoiseFloor();
    }

    radio.instantRssi = -95;
    TEST_ASSERT_TRUE(controller.isCarrierDetected());
}

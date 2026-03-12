#include <unity.h>
#include "hal/i_radio.h"
#include "core/config.h"
#include "mocks/mock_radio.h"
using namespace MiniCore;
class TestRadioEvents : public IRadioEvents {
public:
    bool txDoneCalled{false};
    bool txTimeoutCalled{false};
    bool rxDoneCalled{false};
    bool rxTimeoutCalled{false};
    bool rxErrorCalled{false};
    RxPacket lastPacket{};
        void onTxDone() override { txDoneCalled = true; }
    void onTxTimeout() override { txTimeoutCalled = true; }
    void onRxDone(const RxPacket& packet) override { 
        rxDoneCalled = true; 
        lastPacket = packet;
    }
    void onRxTimeout() override { rxTimeoutCalled = true; }
    void onRxError() override { rxErrorCalled = true; }
        void reset() {
        txDoneCalled = false;
        txTimeoutCalled = false;
        rxDoneCalled = false;
        rxTimeoutCalled = false;
        rxErrorCalled = false;
        lastPacket = {};
    }
};
void test_radio_config_meshcore_defaults() {
    auto config = RadioConfig::meshCoreDefaults();
    TEST_ASSERT_EQUAL_UINT32(Config::RADIO_FREQUENCY_HZ, config.frequencyHz);
    TEST_ASSERT_EQUAL(LoraBandwidth::BW_62_5, config.bandwidth);
    TEST_ASSERT_EQUAL(Config::RADIO_SPREADING_FACTOR, config.spreadingFactor);
    TEST_ASSERT_EQUAL(Config::RADIO_CODING_RATE, config.codingRate);
    TEST_ASSERT_EQUAL(Config::RADIO_PREAMBLE_LENGTH, config.preambleLength);
    TEST_ASSERT_EQUAL(Config::RADIO_SYNC_WORD, config.syncWord);
    TEST_ASSERT_TRUE(config.crcEnabled);
    TEST_ASSERT_FALSE(config.iqInverted);
}
void test_radio_init_stores_config() {
    Mocks::MockRadio radio;
    auto config = RadioConfig::meshCoreDefaults();
        auto result = radio.init(config);
        TEST_ASSERT_TRUE(result.isOk());
    TEST_ASSERT_TRUE(radio.initCalled);
    TEST_ASSERT_EQUAL_UINT32(Config::RADIO_FREQUENCY_HZ, radio.config.frequencyHz);
    TEST_ASSERT_EQUAL(Config::RADIO_SPREADING_FACTOR, radio.config.spreadingFactor);
}
void test_radio_send_stores_data() {
    Mocks::MockRadio radio;
    uint8_t data[] = {0x01, 0x02, 0x03};
        auto result = radio.send(data, 3);
        TEST_ASSERT_TRUE(result.isOk());
    TEST_ASSERT_EQUAL(3, radio.lastSentData.size());
    TEST_ASSERT_EQUAL(0x01, radio.lastSentData[0]);
    TEST_ASSERT_EQUAL(0x02, radio.lastSentData[1]);
    TEST_ASSERT_EQUAL(0x03, radio.lastSentData[2]);
}
void test_radio_event_handler_receives_tx_done() {
    Mocks::MockRadio radio;
    TestRadioEvents events;
    radio.setEventHandler(&events);
        if (radio.eventHandler) {
        radio.eventHandler->onTxDone();
    }
        TEST_ASSERT_TRUE(events.txDoneCalled);
}
void test_radio_event_handler_receives_rx_done() {
    Mocks::MockRadio radio;
    TestRadioEvents events;
    radio.setEventHandler(&events);
    uint8_t data[] = {0xAA, 0xBB};
        radio.simulateRxPacket(data, 2, -50, 10);
        TEST_ASSERT_TRUE(events.rxDoneCalled);
    TEST_ASSERT_EQUAL(-50, events.lastPacket.rssi);
    TEST_ASSERT_EQUAL(10, events.lastPacket.snr);
    TEST_ASSERT_EQUAL(2, events.lastPacket.size);
}
void test_radio_sleep_called() {
    Mocks::MockRadio radio;
        radio.sleep();
        TEST_ASSERT_TRUE(radio.sleepCalled);
}
void test_radio_standby_called() {
    Mocks::MockRadio radio;
        radio.standby();
        TEST_ASSERT_TRUE(radio.standbyCalled);
}
void test_radio_start_receive() {
    Mocks::MockRadio radio;
        auto result = radio.startReceive();
        TEST_ASSERT_TRUE(result.isOk());
    TEST_ASSERT_TRUE(radio.receiveCalled);
}
void test_lora_bandwidth_enum_values() {
    TEST_ASSERT_EQUAL(0, static_cast<uint8_t>(LoraBandwidth::BW_7_8));
    TEST_ASSERT_EQUAL(1, static_cast<uint8_t>(LoraBandwidth::BW_10_4));
    TEST_ASSERT_EQUAL(2, static_cast<uint8_t>(LoraBandwidth::BW_15_6));
    TEST_ASSERT_EQUAL(3, static_cast<uint8_t>(LoraBandwidth::BW_20_8));
    TEST_ASSERT_EQUAL(4, static_cast<uint8_t>(LoraBandwidth::BW_31_25));
    TEST_ASSERT_EQUAL(5, static_cast<uint8_t>(LoraBandwidth::BW_41_7));
    TEST_ASSERT_EQUAL(6, static_cast<uint8_t>(LoraBandwidth::BW_62_5));
    TEST_ASSERT_EQUAL(7, static_cast<uint8_t>(LoraBandwidth::BW_125));
    TEST_ASSERT_EQUAL(8, static_cast<uint8_t>(LoraBandwidth::BW_250));
    TEST_ASSERT_EQUAL(9, static_cast<uint8_t>(LoraBandwidth::BW_500));
}
void test_radio_send_sets_transmitting() {
    Mocks::MockRadio radio;
    uint8_t data[] = {0x01, 0x02, 0x03};
        TEST_ASSERT_FALSE(radio.isTransmitting());
        radio.send(data, 3);
        TEST_ASSERT_TRUE(radio.isTransmitting());
}
void test_radio_tx_done_clears_transmitting() {
    Mocks::MockRadio radio;
    TestRadioEvents events;
    radio.setEventHandler(&events);
    uint8_t data[] = {0x01, 0x02, 0x03};
        radio.send(data, 3);
    TEST_ASSERT_TRUE(radio.isTransmitting());
        radio.simulateTxDone();
        TEST_ASSERT_FALSE(radio.isTransmitting());
    TEST_ASSERT_TRUE(events.txDoneCalled);
}
void test_radio_tx_timeout_clears_transmitting() {
    Mocks::MockRadio radio;
    TestRadioEvents events;
    radio.setEventHandler(&events);
    uint8_t data[] = {0x01, 0x02, 0x03};
        radio.send(data, 3);
    TEST_ASSERT_TRUE(radio.isTransmitting());
        radio.simulateTxTimeout();
        TEST_ASSERT_FALSE(radio.isTransmitting());
}
void test_radio_read_instant_rssi() {
    Mocks::MockRadio radio;

    TEST_ASSERT_EQUAL(-100, radio.readInstantRssi());

    radio.instantRssi = -85;
    TEST_ASSERT_EQUAL(-85, radio.readInstantRssi());
}

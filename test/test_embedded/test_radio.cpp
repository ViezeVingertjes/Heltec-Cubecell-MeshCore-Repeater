#include <unity.h>
#include <Arduino.h>
#include "hal/i_radio.h"
#include "hal/cubecell/cubecell_radio.h"
using namespace MiniCore;
class TestRadioEvents : public IRadioEvents {
public:
    volatile bool rxDone{false};
    volatile bool txDone{false};
    volatile uint8_t lastSize{0};
    volatile int16_t lastRssi{0};
    volatile int8_t lastSnr{0};
    uint8_t lastData[255];
        void onRxDone(const RxPacket& packet) override {
        rxDone = true;
        lastSize = packet.size;
        lastRssi = packet.rssi;
        lastSnr = packet.snr;
        if (packet.size <= sizeof(lastData)) {
            memcpy(lastData, packet.data, packet.size);
        }
    }
    void onRxTimeout() override {}
    void onRxError() override {}
    void onTxDone() override { txDone = true; }
    void onTxTimeout() override {}
        void reset() {
        rxDone = false;
        txDone = false;
        lastSize = 0;
        lastRssi = 0;
        lastSnr = 0;
    }
};
void test_radio_init_succeeds() {
    auto& radio = CubeCellRadio::instance();
    auto config = RadioConfig::meshCoreDefaults();
        auto result = radio.init(config);
        TEST_ASSERT_TRUE(result.isOk());
}
void test_radio_start_receive_succeeds() {
    auto& radio = CubeCellRadio::instance();
    auto config = RadioConfig::meshCoreDefaults();
    radio.init(config);
        auto result = radio.startReceive();
        TEST_ASSERT_TRUE(result.isOk());
}
void test_radio_can_set_event_handler() {
    auto& radio = CubeCellRadio::instance();
    auto config = RadioConfig::meshCoreDefaults();
    radio.init(config);
        TestRadioEvents events;
    radio.setEventHandler(&events);
    radio.startReceive();
        TEST_ASSERT_FALSE(events.rxDone);
}
void test_radio_standby() {
    auto& radio = CubeCellRadio::instance();
    auto config = RadioConfig::meshCoreDefaults();
    radio.init(config);
        radio.standby();
}
void test_radio_sleep() {
    auto& radio = CubeCellRadio::instance();
    auto config = RadioConfig::meshCoreDefaults();
    radio.init(config);
        radio.sleep();
}

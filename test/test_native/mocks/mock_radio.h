#pragma once
#include "hal/i_radio.h"
#include <vector>
#include <cstring>
namespace MiniCore::Mocks {
class MockRadio : public IRadio {
public:
    RadioConfig config{};
    IRadioEvents* eventHandler{nullptr};
    bool initCalled{false};
    bool receiveCalled{false};
    bool standbyCalled{false};
    bool sleepCalled{false};
    bool transmitting_{false};
    Status initResult{};
    Status receiveResult{};
    int16_t rssi{-50};
    int8_t snr{10};
    int16_t instantRssi{-100};
        std::vector<uint8_t> lastSentData;
    Status sendResult{};
        Status init(const RadioConfig& cfg) override {
        initCalled = true;
        config = cfg;
        return initResult;
    }
        void setEventHandler(IRadioEvents* handler) override {
        eventHandler = handler;
    }
        Status startReceive() override {
        receiveCalled = true;
        return receiveResult;
    }
        Status send(const uint8_t* data, uint8_t size) override {
        lastSentData.assign(data, data + size);
        transmitting_ = true;
        return sendResult;
    }
        void setTransmitting(bool value) { transmitting_ = value; }
    [[nodiscard]] bool isTransmitting() const { return transmitting_; }
        void standby() override {
        standbyCalled = true;
    }
        void sleep() override {
        sleepCalled = true;
    }
        int16_t lastRssi() const override { return rssi; }
    int8_t lastSnr() const override { return snr; }
        int16_t readInstantRssi() override { return instantRssi; }
        void simulateRxPacket(const uint8_t* data, uint8_t size, int16_t pktRssi, int8_t pktSnr) {
        if (eventHandler) {
            rssi = pktRssi;
            snr = pktSnr;
            RxPacket pkt{data, size, pktRssi, pktSnr};
            eventHandler->onRxDone(pkt);
        }
    }
        void reset() {
        config = {};
        eventHandler = nullptr;
        initCalled = false;
        receiveCalled = false;
        standbyCalled = false;
        sleepCalled = false;
        transmitting_ = false;
        initResult = {};
        receiveResult = {};
        sendResult = {};
        rssi = -50;
        snr = 10;
        instantRssi = -100;
        lastSentData.clear();
    }
        void simulateTxDone() {
        transmitting_ = false;
        if (eventHandler) {
            eventHandler->onTxDone();
        }
    }
        void simulateTxTimeout() {
        transmitting_ = false;
        if (eventHandler) {
            eventHandler->onTxTimeout();
        }
    }
};
}

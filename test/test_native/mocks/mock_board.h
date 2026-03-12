#pragma once
#include "hal/i_board.h"
namespace MiniCore::Mocks {
class MockBoard : public IBoard {
public:
    DeviceId deviceId{{0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08}};
    uint32_t randomSeed{0x12345678};
    bool resetCalled{false};
    bool interruptsDisabled{false};
    bool watchdogFed{false};
        void reset() override { resetCalled = true; }
        DeviceId getUniqueId() const override { return deviceId; }
        uint32_t getRandomSeed() const override { return randomSeed; }
        void disableInterrupts() override { interruptsDisabled = true; }
        void enableInterrupts() override { interruptsDisabled = false; }
        void feedWatchdog() override { watchdogFed = true; }
        void resetMock() {
        resetCalled = false;
        interruptsDisabled = false;
        watchdogFed = false;
    }
};
}

#pragma once
#include "hal/i_power.h"
namespace MiniCore::Mocks {
class MockPower : public IPower {
public:
    PowerSource powerSource{PowerSource::USB};
    uint16_t batteryMv{3700};
    uint8_t batteryPercent{75};
    SleepMode currentSleepMode{SleepMode::Idle};
    bool vextEnabled{false};
    bool enterSleepCalled{false};
    bool wakeUpCalled{false};
        PowerSource getSource() const override { return powerSource; }
    uint16_t getBatteryMillivolts() const override { return batteryMv; }
    uint8_t getBatteryPercent() const override { return batteryPercent; }
        void enterSleep(SleepMode mode) override {
        enterSleepCalled = true;
        currentSleepMode = mode;
    }
        void wakeUp() override {
        wakeUpCalled = true;
        currentSleepMode = SleepMode::Idle;
    }
        void enableVext(bool enable) override { vextEnabled = enable; }
        void reset() {
        powerSource = PowerSource::USB;
        batteryMv = 3700;
        batteryPercent = 75;
        currentSleepMode = SleepMode::Idle;
        vextEnabled = false;
        enterSleepCalled = false;
        wakeUpCalled = false;
    }
};
}

#pragma once
#include "hal/i_timer.h"
namespace MiniCore::Mocks {
class MockTimer : public ITimer {
public:
    uint32_t currentMillis{0};
    uint32_t currentMicros{0};
    uint32_t delayMsCalled{0};
    uint32_t delayUsCalled{0};
        uint32_t millis() const override { return currentMillis; }
    uint32_t micros() const override { return currentMicros; }
        void delayMs(uint32_t ms) override { 
        delayMsCalled += ms;
        currentMillis += ms;
    }
        void delayUs(uint32_t us) override { 
        delayUsCalled += us;
        currentMicros += us;
    }
        void advance(uint32_t ms) {
        currentMillis += ms;
        currentMicros += ms * 1000;
    }
        void reset() {
        currentMillis = 0;
        currentMicros = 0;
        delayMsCalled = 0;
        delayUsCalled = 0;
    }
};
}

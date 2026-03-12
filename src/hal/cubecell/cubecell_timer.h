#pragma once
#include <Arduino.h>
#include "hal/i_timer.h"
namespace MiniCore {
class CubeCellTimer : public ITimer {
public:
    uint32_t millis() const override { return ::millis(); }
    uint32_t micros() const override { return ::micros(); }
    void delayMs(uint32_t ms) override { delay(ms); }
    void delayUs(uint32_t us) override { delayMicroseconds(us); }
};
}

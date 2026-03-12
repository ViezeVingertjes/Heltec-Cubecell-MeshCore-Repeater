#pragma once
#include <cstdint>
namespace MiniCore {
using TimerCallback = void(*)();
class IScheduler {
public:
    virtual ~IScheduler() = default;
        virtual void scheduleOnce(uint32_t delayMs, TimerCallback callback) = 0;
    virtual void scheduleRepeating(uint32_t intervalMs, TimerCallback callback) = 0;
    virtual void cancel(TimerCallback callback) = 0;
        virtual uint32_t millis() const = 0;
    virtual uint32_t micros() const = 0;
        virtual void delayMs(uint32_t ms) = 0;
    virtual void delayUs(uint32_t us) = 0;
};
}

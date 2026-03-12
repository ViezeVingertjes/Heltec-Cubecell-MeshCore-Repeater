#pragma once
#include <cstdint>
namespace MiniCore {
class ITimer {
public:
    virtual ~ITimer() = default;
        virtual uint32_t millis() const = 0;
    virtual uint32_t micros() const = 0;
    virtual void delayMs(uint32_t ms) = 0;
    virtual void delayUs(uint32_t us) = 0;
};
}

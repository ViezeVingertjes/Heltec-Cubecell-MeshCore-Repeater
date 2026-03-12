#pragma once
#include <cstdint>
namespace MiniCore {
struct DeviceId {
    uint8_t bytes[8];
};
class IBoard {
public:
    virtual ~IBoard() = default;
        virtual void reset() = 0;
    virtual DeviceId getUniqueId() const = 0;
    virtual uint32_t getRandomSeed() const = 0;
        virtual void disableInterrupts() = 0;
    virtual void enableInterrupts() = 0;
        virtual void feedWatchdog() = 0;
};
}

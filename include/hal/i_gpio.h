#pragma once
#include <cstdint>
namespace MiniCore {
enum class PinMode : uint8_t {
    Input,
    Output,
    InputPullup,
    InputPulldown
};
enum class PinState : uint8_t {
    Low = 0,
    High = 1
};
class IGpio {
public:
    virtual ~IGpio() = default;
        virtual void setMode(uint8_t pin, PinMode mode) = 0;
    virtual void write(uint8_t pin, PinState state) = 0;
    virtual PinState read(uint8_t pin) = 0;
};
}

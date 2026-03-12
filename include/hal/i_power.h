#pragma once
#include <cstdint>
namespace MiniCore {
enum class PowerSource : uint8_t {
    USB,
    Battery
};
enum class SleepMode : uint8_t {
    Idle,
    LightSleep,
    DeepSleep
};
class IPower {
public:
    virtual ~IPower() = default;
        virtual PowerSource getSource() const = 0;
    virtual uint16_t getBatteryMillivolts() const = 0;
    virtual uint8_t getBatteryPercent() const = 0;
        virtual void enterSleep(SleepMode mode) = 0;
    virtual void wakeUp() = 0;
        virtual void enableVext(bool enable) = 0;
};
}

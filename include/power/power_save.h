#pragma once
#include <cstdint>
#include "core/config.h"
#include "hal/i_radio.h"
#include "hal/i_timer.h"
namespace MiniCore {
using Config::POWER_SAVE_IDLE_TIMEOUT_MS;
using Config::POWER_SAVE_SLEEP_DURATION_MS;
enum class PowerSaveState : uint8_t {
    Active,
    Sleeping
};
struct PowerSaveConfig {
    uint32_t idleTimeoutMs;
    uint32_t sleepDurationMs;
    static PowerSaveConfig defaults() {
        return {
            .idleTimeoutMs = POWER_SAVE_IDLE_TIMEOUT_MS,
            .sleepDurationMs = POWER_SAVE_SLEEP_DURATION_MS
        };
    }
};
class PowerSaveController {
public:
    PowerSaveController(IRadio& radio, ITimer& timer, const PowerSaveConfig& config = PowerSaveConfig::defaults());
    void process();
    void onPacketRepeated();
    void setEnabled(bool enabled) { enabled_ = enabled; }
    [[nodiscard]] bool isEnabled() const { return enabled_; }
    [[nodiscard]] bool isSleeping() const { return state_ == PowerSaveState::Sleeping; }
    [[nodiscard]] PowerSaveState state() const { return state_; }
    [[nodiscard]] uint32_t lastRepeatTime() const { return lastRepeatTime_; }
private:
    void enterSleep();
    void wake();
    IRadio& radio_;
    ITimer& timer_;
    PowerSaveConfig config_;
    PowerSaveState state_{PowerSaveState::Active};
    uint32_t lastRepeatTime_{0};
    uint32_t sleepStartTime_{0};
    bool enabled_{true};
};
}

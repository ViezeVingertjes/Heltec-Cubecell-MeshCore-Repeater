#include "power/power_save.h"
namespace MiniCore {
PowerSaveController::PowerSaveController(IRadio& radio, ITimer& timer, const PowerSaveConfig& config)
    : radio_(radio)
    , timer_(timer)
    , config_(config)
{
}
void PowerSaveController::process() {
    if (!enabled_) {
        return;
    }
        uint32_t currentTime = timer_.millis();
        if (state_ == PowerSaveState::Active) {
        uint32_t elapsed = currentTime - lastRepeatTime_;
        if (elapsed >= config_.idleTimeoutMs) {
            enterSleep();
        }
    } else {
        uint32_t sleepElapsed = currentTime - sleepStartTime_;
        if (sleepElapsed >= config_.sleepDurationMs) {
            wake();
        }
    }
}
void PowerSaveController::onPacketRepeated() {
    lastRepeatTime_ = timer_.millis();
}
void PowerSaveController::enterSleep() {
    state_ = PowerSaveState::Sleeping;
    sleepStartTime_ = timer_.millis();
    radio_.sleep();
}
void PowerSaveController::wake() {
    state_ = PowerSaveState::Active;
    lastRepeatTime_ = timer_.millis();
    radio_.startReceive();
}
}

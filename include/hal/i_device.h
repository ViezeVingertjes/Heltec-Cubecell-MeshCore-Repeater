#pragma once
#include "i_board.h"
#include "i_crypto.h"
#include "i_gpio.h"
#include "i_log.h"
#include "i_power.h"
#include "i_radio.h"
#include "i_rng.h"
#include "i_storage.h"
#include "i_timer.h"
namespace MiniCore {
class IDevice {
public:
    virtual ~IDevice() = default;

    virtual Status init() = 0;
    virtual const char* name() const = 0;

        virtual IBoard& board() = 0;
    virtual ILog& log() = 0;
    virtual ITimer& timer() = 0;
    virtual IRng& rng() = 0;
    virtual IStorage& storage() = 0;
    virtual IRadio& radio() = 0;

        virtual IGpio* gpio() { return nullptr; }
    virtual IPower* power() { return nullptr; }

    virtual void processLoop() = 0;
    virtual void idle() = 0;
    virtual Status factoryReset() = 0;
    virtual bool checkFactoryResetRequest(uint32_t timeoutMs) = 0;
    virtual void reboot() = 0;
};
}

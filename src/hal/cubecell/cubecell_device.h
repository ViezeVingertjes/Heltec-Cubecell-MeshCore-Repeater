#pragma once
#ifdef __asr650x__
#include "hal/i_device.h"
#include "cubecell_radio.h"
#include "cubecell_log.h"
#include "cubecell_rng.h"
#include "cubecell_timer.h"
#include "flash_storage.h"
namespace MiniCore {
class CubeCellBoard : public IBoard {
public:
    void reset() override;
    DeviceId getUniqueId() const override;
    uint32_t getRandomSeed() const override;
    void disableInterrupts() override;
    void enableInterrupts() override;
    void feedWatchdog() override;
};
class CubeCellDevice : public IDevice {
public:
    static CubeCellDevice& instance();
        Status init() override;
    const char* name() const override { return "CubeCell"; }
        IBoard& board() override { return board_; }
    ILog& log() override { return log_; }
    ITimer& timer() override { return timer_; }
    IRng& rng() override { return rng_; }
    IStorage& storage() override { return storage_; }
    IRadio& radio() override { return CubeCellRadio::instance(); }
        void processLoop() override;
    void idle() override;
    Status factoryReset() override;
    bool checkFactoryResetRequest(uint32_t timeoutMs) override;
    void reboot() override;
private:
    CubeCellDevice() = default;
        CubeCellBoard board_;
    CubeCellLog log_;
    CubeCellTimer timer_;
    CubeCellRng rng_;
    FlashStorage storage_;
};
}
#endif

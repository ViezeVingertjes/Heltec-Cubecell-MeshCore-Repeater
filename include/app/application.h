#pragma once
#include <cstdint>
#include <memory>
#include "core/config.h"
#include "crypto/crypto_types.h"
#include "packet/packet_history.h"
#include "routing/repeater.h"
#include "queue/tx_queue.h"
#include "time/time_sync.h"
#include "advert/advert.h"
#include "hal/i_device.h"
#include "hal/i_radio.h"
#include "routing/packet_router.h"
#include "radio/radio_controller.h"
namespace MiniCore {
class PowerSaveController;
class Ed25519Crypto;
struct ApplicationHandlers;
struct ApplicationContext {
    IDevice& device;
    TxQueue& txQueue;
    PacketRouter* packetRouter;
    RadioController* radioController;
    PowerSaveController* powerSaveController;
    TimeSynchronizer& timeSync;
    uint32_t& packetCount;
    uint8_t& csRetryCount;
    bool hasIdentity;
    uint8_t selfHash;
    const LocalIdentity& localIdentity;
};
class Application {
public:
    explicit Application(IDevice& device);
    ~Application();
    void init();
    void loop();
    ApplicationContext context() {
        return ApplicationContext{
            device_,
            txQueue_,
            packetRouter_.get(),
            radioController_.get(),
            powerSaveController_.get(),
            timeSync_,
            packetCount_,
            csRetryCount_,
            hasIdentity_,
            selfHash_,
            localIdentity_
        };
    }
    void setTxHandlerPendingEntry(TxEntry* entry);
    void logInfo(const char* fmt, ...);
    void logError(const char* fmt, ...);
private:
    IDevice& device_;
    uint32_t packetCount_{0};
    uint8_t selfHash_{0};
    LocalIdentity localIdentity_{};
    bool hasIdentity_{false};
    uint32_t lastNoiseFloorLogTime_{0};
    uint32_t lastNoiseFloorSampleTime_{0};
    bool noiseFloorCalibrated_{false};
    PacketHistory packetHistory_;
    RepeaterConfig repeaterConfig_;
    Repeater repeater_;
    TxQueue txQueue_;
    AdvertScheduler advertScheduler_;
    TimeSynchronizer timeSync_;
    char nodeName_[Config::NODE_NAME_BUF_SIZE]{0};
    uint8_t csRetryCount_{0};
    std::unique_ptr<Ed25519Crypto> crypto_;
    std::unique_ptr<ApplicationHandlers> handlers_;
    std::unique_ptr<PacketRouter> packetRouter_;
    std::unique_ptr<RadioController> radioController_;
    std::unique_ptr<PowerSaveController> powerSaveController_;
    IRadioControllerEvents* txHandler_{nullptr};
    IPacketRouterEvents* routerHandler_{nullptr};
    IRadioEvents* radioHandler_{nullptr};
};
}

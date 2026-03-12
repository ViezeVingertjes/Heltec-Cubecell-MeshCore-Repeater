#pragma once
#include <cstdint>
#include <cstddef>
#include "core/config.h"
#include "core/result.h"
#include "hal/i_radio.h"
#include "hal/i_timer.h"
#include "util/noise_floor_estimator.h"
namespace MiniCore {
using Config::CS_MAX_RETRIES;
using Config::CS_RETRY_DELAY_MS;
enum class RadioControllerState : uint8_t {
    Idle,
    Transmitting,
    AwaitingRetry
};
struct CarrierSenseConfig {
    uint8_t maxRetries;
    uint32_t retryDelayMs;
    static CarrierSenseConfig defaults() {
        return {
            .maxRetries = CS_MAX_RETRIES,
            .retryDelayMs = CS_RETRY_DELAY_MS
        };
    }
};
class IRadioControllerEvents {
public:
    virtual ~IRadioControllerEvents() = default;
    virtual void onTransmitComplete(bool success) = 0;
    virtual void onChannelBusy() = 0;
};
class RadioController : public IRadioEvents {
public:
    RadioController(IRadio& radio, ITimer& timer, const CarrierSenseConfig& config = CarrierSenseConfig::defaults());
    Status transmitWithCS(const uint8_t* data, uint8_t size);
    void setEventHandler(IRadioControllerEvents* handler);
    void setRxEventHandler(IRadioEvents* handler);
    void process(uint32_t currentTimeMs);
    [[nodiscard]] RadioControllerState state() const { return state_; }
    [[nodiscard]] uint8_t retryCount() const { return retryCount_; }
    [[nodiscard]] uint8_t pendingSize() const { return pendingSize_; }
    [[nodiscard]] bool isBusy() const;
    void enableCarrierSense(bool enable);
    void configureCarrierSense(LoraBandwidth bandwidth);
    [[nodiscard]] bool isCarrierSenseEnabled() const { return carrierSenseEnabled_; }
    void updateNoiseFloor();
    [[nodiscard]] bool isNoiseFloorValid() const;
    [[nodiscard]] int16_t getNoiseFloor() const;
    [[nodiscard]] int16_t getThreshold() const;
    [[nodiscard]] bool isCarrierDetected();
    void onRxDone(const RxPacket& packet) override;
    void onRxTimeout() override;
    void onRxError() override;
    void onTxDone() override;
    void onTxTimeout() override;
private:
    void startCarrierSenseCheck();
    void performTransmit();
    void scheduleRetry(uint32_t currentTimeMs);
    void reset();
    bool checkCarrierSense();
    void notifyChannelBusy();
    void notifyTxComplete(bool success);
    void notifyRxDone(const RxPacket& packet);
    void notifyRxTimeout();
    void notifyRxError();
    IRadio& radio_;
    ITimer& timer_;
    CarrierSenseConfig config_;
    IRadioControllerEvents* eventHandler_{nullptr};
    IRadioEvents* rxEventHandler_{nullptr};
    RadioControllerState state_{RadioControllerState::Idle};
    const uint8_t* pendingData_{nullptr};
    uint8_t pendingSize_{0};
    uint8_t retryCount_{0};
    uint32_t retryScheduledTime_{0};
    bool carrierSenseEnabled_{false};
    NoiseFloorEstimator noiseFloorEstimator_;
};
}

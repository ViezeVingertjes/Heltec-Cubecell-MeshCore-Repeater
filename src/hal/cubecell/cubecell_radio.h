#pragma once
#include "hal/i_radio.h"
#ifdef __asr650x__
namespace MiniCore {
class CubeCellRadio : public IRadio {
public:
    static CubeCellRadio& instance();
        Status init(const RadioConfig& config) override;
    void setEventHandler(IRadioEvents* handler) override;
    Status startReceive() override;
    Status send(const uint8_t* data, uint8_t size) override;
    void standby() override;
    void sleep() override;
    int16_t lastRssi() const override;
    int8_t lastSnr() const override;
    int16_t readInstantRssi() override;
        IRadioEvents* getEventHandler() const { return eventHandler_; }
        void setTransmitting(bool value) { transmitting_ = value; }
    [[nodiscard]] bool isTransmitting() const { return transmitting_; }
private:
    CubeCellRadio() = default;
        static uint8_t bandwidthToValue(LoraBandwidth bw);
        IRadioEvents* eventHandler_{nullptr};
    RadioConfig config_{};
    bool initialized_{false};
    bool transmitting_{false};
};
}
#endif

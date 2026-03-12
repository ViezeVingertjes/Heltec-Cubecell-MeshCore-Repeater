#pragma once
#include <cstdint>
#include <cstddef>
#include "core/config.h"
#include "core/result.h"
namespace MiniCore {
enum class LoraBandwidth : uint8_t {
    BW_7_8 = 0,
    BW_10_4 = 1,
    BW_15_6 = 2,
    BW_20_8 = 3,
    BW_31_25 = 4,
    BW_41_7 = 5,
    BW_62_5 = 6,
    BW_125 = 7,
    BW_250 = 8,
    BW_500 = 9
};
struct RadioConfig {
    uint32_t frequencyHz;
    LoraBandwidth bandwidth;
    uint8_t spreadingFactor;
    uint8_t codingRate;
    uint8_t preambleLength;
    int8_t txPowerDbm;
    uint16_t syncWord;
    bool crcEnabled;
    bool iqInverted;
        static RadioConfig meshCoreDefaults() {
        return {
            .frequencyHz = Config::RADIO_FREQUENCY_HZ,
            .bandwidth = LoraBandwidth::BW_62_5,
            .spreadingFactor = Config::RADIO_SPREADING_FACTOR,
            .codingRate = Config::RADIO_CODING_RATE,
            .preambleLength = Config::RADIO_PREAMBLE_LENGTH,
            .txPowerDbm = Config::RADIO_TX_POWER_DBM,
            .syncWord = Config::RADIO_SYNC_WORD,
            .crcEnabled = Config::RADIO_CRC_ENABLED,
            .iqInverted = Config::RADIO_IQ_INVERTED
        };
    }
};
struct RxPacket {
    const uint8_t* data;
    uint8_t size;
    int16_t rssi;
    int8_t snr;
};
class IRadioEvents {
public:
    virtual ~IRadioEvents() = default;
    virtual void onRxDone(const RxPacket& packet) = 0;
    virtual void onRxTimeout() = 0;
    virtual void onRxError() = 0;
    virtual void onTxDone() = 0;
    virtual void onTxTimeout() = 0;
};
class IRadio {
public:
    virtual ~IRadio() = default;
        virtual Status init(const RadioConfig& config) = 0;
    virtual void setEventHandler(IRadioEvents* handler) = 0;
        virtual Status startReceive() = 0;
    virtual Status send(const uint8_t* data, uint8_t size) = 0;
    virtual void standby() = 0;
    virtual void sleep() = 0;
        virtual int16_t lastRssi() const = 0;
    virtual int8_t lastSnr() const = 0;
        virtual int16_t readInstantRssi() = 0;
};
}

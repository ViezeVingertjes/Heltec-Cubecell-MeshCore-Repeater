#include "cubecell_radio.h"
#include "core/config.h"
#ifdef __asr650x__
#include "LoRaWan_APP.h"
namespace MiniCore {
static uint8_t rxBuffer[Config::MAX_MTU_SIZE];
static RadioEvents_t radioEvents;
static int16_t lastRxRssi = 0;
static int8_t lastRxSnr = 0;
static void onRxDone(uint8_t* payload, uint16_t size, int16_t rssi, int8_t snr) {
    if (size > sizeof(rxBuffer)) size = sizeof(rxBuffer);
    memcpy(rxBuffer, payload, size);
    lastRxRssi = rssi;
    lastRxSnr = snr;
        auto* handler = CubeCellRadio::instance().getEventHandler();
    if (handler) {
        RxPacket pkt{rxBuffer, static_cast<uint8_t>(size), rssi, snr};
        handler->onRxDone(pkt);
    }
        Radio.RxBoosted(0);
}
static void onRxTimeout() {
    auto* handler = CubeCellRadio::instance().getEventHandler();
    if (handler) {
        handler->onRxTimeout();
    }
    Radio.RxBoosted(0);
}
static void onRxError() {
    auto* handler = CubeCellRadio::instance().getEventHandler();
    if (handler) {
        handler->onRxError();
    }
    Radio.RxBoosted(0);
}
static void onTxDone() {
    CubeCellRadio::instance().setTransmitting(false);
    auto* handler = CubeCellRadio::instance().getEventHandler();
    if (handler) {
        handler->onTxDone();
    }
    Radio.RxBoosted(0);
}
static void onTxTimeout() {
    CubeCellRadio::instance().setTransmitting(false);
    auto* handler = CubeCellRadio::instance().getEventHandler();
    if (handler) {
        handler->onTxTimeout();
    }
    Radio.RxBoosted(0);
}
CubeCellRadio& CubeCellRadio::instance() {
    static CubeCellRadio inst;
    return inst;
}
uint8_t CubeCellRadio::bandwidthToValue(LoraBandwidth bw) {
    switch (bw) {
        case LoraBandwidth::BW_7_8:   return 0;
        case LoraBandwidth::BW_10_4:  return 8;
        case LoraBandwidth::BW_15_6:  return 1;
        case LoraBandwidth::BW_20_8:  return 9;
        case LoraBandwidth::BW_31_25: return 2;
        case LoraBandwidth::BW_41_7:  return 10;
        case LoraBandwidth::BW_62_5:  return 3;
        case LoraBandwidth::BW_125:   return 4;
        case LoraBandwidth::BW_250:   return 5;
        case LoraBandwidth::BW_500:   return 6;
    }
    return 3;
}
Status CubeCellRadio::init(const RadioConfig& config) {
    config_ = config;
        radioEvents.RxDone = onRxDone;
    radioEvents.RxTimeout = onRxTimeout;
    radioEvents.RxError = onRxError;
    radioEvents.TxDone = onTxDone;
    radioEvents.TxTimeout = onTxTimeout;
        Radio.Init(&radioEvents);
    Radio.SetChannel(config_.frequencyHz);
        uint8_t bwValue = bandwidthToValue(config_.bandwidth);
        Radio.SetRxConfig(
        MODEM_LORA,
        bwValue,
        config_.spreadingFactor,
        config_.codingRate,
        0,
        config_.preambleLength,
        0,
        false,
        0,
        config_.crcEnabled,
        false,
        0,
        config_.iqInverted,
        true
    );
        Radio.SetSyncWord(static_cast<uint8_t>(config_.syncWord & 0xFF));
        Radio.SetTxConfig(
        MODEM_LORA,
        config_.txPowerDbm,
        0,
        bwValue,
        config_.spreadingFactor,
        config_.codingRate,
        config_.preambleLength,
        false,
        config_.crcEnabled,
        false,
        0,
        config_.iqInverted,
        3000
    );
        initialized_ = true;
    return Status();
}
void CubeCellRadio::setEventHandler(IRadioEvents* handler) {
    eventHandler_ = handler;
}
Status CubeCellRadio::startReceive() {
    if (!initialized_) {
        return ErrorCode::NotInitialized;
    }
    Radio.RxBoosted(0);
    return Status();
}
Status CubeCellRadio::send(const uint8_t* data, uint8_t size) {
    if (!initialized_) {
        return ErrorCode::NotInitialized;
    }
    transmitting_ = true;
    Radio.Send(const_cast<uint8_t*>(data), size);
    return Status();
}
void CubeCellRadio::standby() {
    Radio.Standby();
}
void CubeCellRadio::sleep() {
    Radio.Sleep();
}
int16_t CubeCellRadio::lastRssi() const {
    return lastRxRssi;
}
int8_t CubeCellRadio::lastSnr() const {
    return lastRxSnr;
}
int16_t CubeCellRadio::readInstantRssi() {
    return Radio.Rssi(MODEM_LORA);
}
}
#endif

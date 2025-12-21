#include "LoRaTransmitter.h"
#include "../core/Logger.h"
#include <Arduino.h>

extern RadioEvents_t radioEvents;

LoRaTransmitter &LoRaTransmitter::getInstance() {
  static LoRaTransmitter instance;
  return instance;
}

void LoRaTransmitter::initialize() {
  LOG_DEBUG("Initializing LoRa transmitter");
  nextAllowedTxTime = 0;
  LOG_INFO("LoRa transmitter ready");
}

void LoRaTransmitter::registerTxCallbacks() {
  LOG_DEBUG("Registering TX event callbacks");
  radioEvents.TxDone = onTxDone;
  radioEvents.TxTimeout = onTxTimeout;
}

void LoRaTransmitter::onTxDone() {
  LoRaTransmitter &tx = getInstance();
  tx.notifyTxComplete(true);
  LOG_DEBUG("TX complete, returning to RX");
  Radio.RxBoosted(0);  // Return to boosted RX mode
}

void LoRaTransmitter::onTxTimeout() {
  LoRaTransmitter &tx = getInstance();
  tx.notifyTxComplete(false);
  LOG_WARN("TX timeout, returning to RX");
  Radio.RxBoosted(0);  // Return to boosted RX mode
}

bool LoRaTransmitter::transmit(const uint8_t *data, uint16_t length) {
  if (transmitting) {
    LOG_WARN("Transmit rejected - already transmitting");
    return false;
  }

  if (!canTransmitNow()) {
    uint32_t waitTime = nextAllowedTxTime - millis();
    LOG_DEBUG_FMT("In silence period, must wait %lu ms", waitTime);
    return false;
  }

  if (data == nullptr || length == 0 || length > 255) {
    LOG_ERROR("Invalid transmit parameters");
    failureCount++;
    return false;
  }

  LOG_DEBUG_FMT("Transmitting %d bytes", length);

  transmitting = true;
  txStartTime = millis();
  transmitCount++;
  Radio.Send(const_cast<uint8_t *>(data), length);

  return true;
}

bool LoRaTransmitter::canTransmitNow() const {
  return millis() >= nextAllowedTxTime;
}

void LoRaTransmitter::notifyTxComplete(bool success) {
  transmitting = false;

  if (success) {
    uint32_t airtime = millis() - txStartTime;
    totalAirtimeMs += airtime;
    
    // Duty cycle enforcement disabled
    nextAllowedTxTime = millis();

    LOG_DEBUG_FMT("TX complete, airtime: %lu ms", airtime);
  } else {
    failureCount++;
  }
}

uint32_t LoRaTransmitter::estimateAirtime(uint16_t packetLength) {
  // Pre-computed constants for SF8, BW=62.5kHz, CR=4/4, Preamble=16
  // Symbol duration = (2^SF / BW) * 1000 ms = (256 / 62500) * 1000 = 4.096 ms
  // Preamble symbols = 16 + 4.25 = 20.25 symbols
  
  // For better accuracy with integer math, work in microseconds (scale by 1000)
  // Symbol duration in microseconds: 4096 us
  constexpr uint32_t SYMBOL_DURATION_US = 4096;
  
  // Preamble time in microseconds: 20.25 * 4096 = 82944 us
  constexpr uint32_t PREAMBLE_TIME_US = 82944;
  
  // Payload calculation: 8 + ceil((8*PL - 32 + 28 + 16) / 32) * 8
  // Simplified: 8 + ceil((8*PL + 12) / 32) * 8
  uint32_t numerator = (8 * packetLength + 12);
  uint32_t payloadSymbols = 8 + (((numerator + 31) / 32) * 8);  // ceil division
  
  uint32_t payloadTime = payloadSymbols * SYMBOL_DURATION_US;
  uint32_t totalTime = (PREAMBLE_TIME_US + payloadTime) / 1000;  // Convert to ms
  
  return totalTime;
}

void LoRaTransmitter::resetStats() {
  transmitCount = 0;
  failureCount = 0;
  totalAirtimeMs = 0;
  LOG_INFO("TX statistics reset");
}
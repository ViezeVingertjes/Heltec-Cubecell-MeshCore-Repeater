#pragma once

#include "../core/Config.h"
#include "../core/PacketDecoder.h"
#include "LoRaWan_APP.h"

class LoRaTransmitter {
public:
  static LoRaTransmitter &getInstance();

  void initialize();
  bool transmit(const uint8_t *data, uint16_t length);
  bool isTransmitting() const { return transmitting; }
  bool canTransmitNow() const;
  void notifyTxComplete(bool success);

  uint32_t getTransmitCount() const { return transmitCount; }
  uint32_t getFailureCount() const { return failureCount; }
  uint32_t getTotalAirtimeMs() const { return totalAirtimeMs; }
  void resetStats();

  static uint32_t estimateAirtime(uint16_t packetLength);
  
  static void registerTxCallbacks();

private:
  LoRaTransmitter()
      : transmitting(false), transmitCount(0), failureCount(0),
        totalAirtimeMs(0), txStartTime(0), nextAllowedTxTime(0) {}
  
  static void onTxDone();
  static void onTxTimeout();

  bool transmitting;
  uint32_t transmitCount;
  uint32_t failureCount;
  uint32_t totalAirtimeMs;
  uint32_t txStartTime;
  uint32_t nextAllowedTxTime;

  LoRaTransmitter(const LoRaTransmitter &) = delete;
  LoRaTransmitter &operator=(const LoRaTransmitter &) = delete;
};

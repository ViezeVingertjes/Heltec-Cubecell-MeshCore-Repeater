#pragma once

#include "../../core/Config.h"
#include "../../core/Logger.h"
#include "../../radio/LoRaTransmitter.h"
#include "../PacketDispatcher.h"

namespace MeshCore {

struct DelayedPacket {
  uint8_t encodedPacket[256];
  uint16_t packetLength;
  uint32_t scheduledTime;
  bool valid;
};

class PacketForwarder : public IPacketProcessor {
public:
  PacketForwarder() : forwardedCount(0), droppedCount(0), delayedCount(0) {
    for (size_t i = 0; i < DELAY_QUEUE_SIZE; ++i) {
      delayQueue[i].valid = false;
    }
  }
  ~PacketForwarder() override = default;

  ProcessResult processPacket(const PacketEvent &event,
                              ProcessingContext &ctx) override;
  const char *getName() const override { return "PacketForwarder"; }
  uint8_t getPriority() const override { return 50; }

  void loop();

  uint32_t getForwardedCount() const { return forwardedCount; }
  uint32_t getDroppedCount() const { return droppedCount; }
  uint32_t getDelayedCount() const { return delayedCount; }

private:
  static constexpr size_t DELAY_QUEUE_SIZE = 4;

  uint32_t forwardedCount;
  uint32_t droppedCount;
  uint32_t delayedCount;
  DelayedPacket delayQueue[DELAY_QUEUE_SIZE];

  bool shouldForward(const DecodedPacket &packet, int16_t rssi,
                     const ProcessingContext &ctx);
  bool transmitPacket(const uint8_t *rawPacket, uint16_t length);
  bool addNodeToPath(DecodedPacket &packet);

  float calculatePacketScore(int8_t snr) const;
  uint32_t calculateRxDelay(float score, uint32_t airtime) const;
  uint32_t calculateTxJitter(uint32_t airtime) const;

  bool enqueueDelayed(const uint8_t *encodedPacket, uint16_t length,
                      uint32_t delayMs);
  bool processDelayQueue();
};

} // namespace MeshCore

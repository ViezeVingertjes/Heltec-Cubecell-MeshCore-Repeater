#pragma once

#include "../../core/Config.h"
#include "../../core/Logger.h"
#include "../../core/Result.h"
#include "../../core/containers/PriorityQueue.h"
#include "../../radio/LoRaTransmitter.h"
#include "../PacketDispatcher.h"

namespace MeshCore {

/**
 * Delayed packet for priority queue
 */
struct DelayedPacket {
  uint8_t encodedPacket[Config::Forwarding::MAX_ENCODED_PACKET_SIZE];
  uint16_t packetLength;
  uint32_t scheduledTime;
  bool valid;

  DelayedPacket()
      : packetLength(0), scheduledTime(0), valid(false) {
    memset(encodedPacket, 0, sizeof(encodedPacket));
  }
};

/**
 * PacketForwarder handles mesh packet forwarding with adaptive delays.
 * Better signal quality results in shorter delays, allowing nodes with
 * better reception to forward first.
 */
class PacketForwarder : public IPacketProcessor {
public:
  PacketForwarder() : forwardedCount(0), droppedCount(0), delayedCount(0) {}
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
  static constexpr size_t DELAY_QUEUE_SIZE =
      Config::Forwarding::DELAY_QUEUE_SIZE;

  uint32_t forwardedCount;
  uint32_t droppedCount;
  uint32_t delayedCount;

  PriorityQueue<DelayedPacket, DELAY_QUEUE_SIZE, uint32_t> delayQueue;

  Result<void> shouldForward(const DecodedPacket &packet, int16_t rssi,
                             const ProcessingContext &ctx);
  Result<void> transmitPacket(const uint8_t *rawPacket, uint16_t length);
  Result<void> addNodeToPath(DecodedPacket &packet);

  float calculatePacketScore(int8_t snr) const;
  uint32_t calculateRxDelay(float score, uint32_t airtime) const;
  uint32_t calculateTxJitter(uint32_t airtime) const;

  Result<void> enqueueDelayed(const uint8_t *encodedPacket, uint16_t length,
                              uint32_t delayMs);
  bool processDelayQueue();
};

} // namespace MeshCore

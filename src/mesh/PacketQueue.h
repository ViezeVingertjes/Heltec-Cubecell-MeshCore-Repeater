#pragma once

#include "../core/Config.h"
#include "../core/PacketDecoder.h"

namespace MeshCore {

struct QueuedPacket {
  DecodedPacket packet;
  int16_t rssi;
  int8_t snr;
  uint32_t timestamp;
  bool valid;
};

class PacketQueue {
public:
  PacketQueue();

  bool enqueue(const DecodedPacket &packet, int16_t rssi, int8_t snr,
               uint32_t timestamp);
  bool dequeue(QueuedPacket &outPacket);

  bool isEmpty() const { return count == 0; }
  bool isFull() const { return count >= QUEUE_SIZE; }
  size_t getCount() const { return count; }
  uint32_t getDroppedCount() const { return droppedCount; }

private:
  static constexpr size_t QUEUE_SIZE = Config::Queue::PACKET_QUEUE_SIZE;

  QueuedPacket queue[QUEUE_SIZE];
  size_t readIndex;
  size_t writeIndex;
  size_t count;
  uint32_t droppedCount;
};

} // namespace MeshCore

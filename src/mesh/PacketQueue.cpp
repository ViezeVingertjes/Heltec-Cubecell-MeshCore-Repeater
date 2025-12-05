#include "PacketQueue.h"
#include "../core/Logger.h"
#include <string.h>

namespace MeshCore {

PacketQueue::PacketQueue()
    : readIndex(0), writeIndex(0), count(0), droppedCount(0) {
  for (size_t i = 0; i < QUEUE_SIZE; ++i) {
    queue[i].valid = false;
  }
}

bool PacketQueue::enqueue(const DecodedPacket &packet, int16_t rssi, int8_t snr,
                          uint32_t timestamp) {
  if (isFull()) {
    droppedCount++;
    LOG_WARN_FMT("Packet queue full, dropping packet (total dropped: %lu)",
                 droppedCount);
    return false;
  }

  memcpy(&queue[writeIndex].packet, &packet, sizeof(DecodedPacket));
  queue[writeIndex].rssi = rssi;
  queue[writeIndex].snr = snr;
  queue[writeIndex].timestamp = timestamp;
  queue[writeIndex].valid = true;

  writeIndex = (writeIndex + 1) % QUEUE_SIZE;
  count++;

  return true;
}

bool PacketQueue::dequeue(QueuedPacket &outPacket) {
  if (isEmpty()) {
    return false;
  }

  if (!queue[readIndex].valid) {
    readIndex = (readIndex + 1) % QUEUE_SIZE;
    count--;
    return false;
  }

  memcpy(&outPacket, &queue[readIndex], sizeof(QueuedPacket));
  queue[readIndex].valid = false;

  readIndex = (readIndex + 1) % QUEUE_SIZE;
  count--;

  return true;
}

} // namespace MeshCore

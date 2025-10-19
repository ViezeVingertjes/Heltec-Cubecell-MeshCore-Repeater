#include "PacketStats.h"

namespace MeshCore {

ProcessResult PacketStats::processPacket(const PacketEvent &event,
                                         ProcessingContext &ctx) {
  totalPackets++;

  if (event.packet.payloadType == PayloadType::ADVERT) {
    advertPackets++;
  }

  if (event.rssi > maxRssi)
    maxRssi = event.rssi;
  if (event.rssi < minRssi)
    minRssi = event.rssi;
  rssiSum += event.rssi;

  return ProcessResult::CONTINUE;
}

void PacketStats::printStats() {
  LOG_INFO("=== Packet Statistics ===");
  LOG_INFO_FMT("Total packets: %lu", totalPackets);
  LOG_INFO_FMT("Advertisement packets: %lu", advertPackets);

  if (totalPackets > 0) {
    int16_t avgRssi = rssiSum / totalPackets;
    LOG_INFO_FMT("RSSI: min=%d, max=%d, avg=%d dBm", minRssi, maxRssi, avgRssi);
  }
  LOG_INFO("========================");
}

void PacketStats::resetStats() {
  totalPackets = 0;
  advertPackets = 0;
  minRssi = 0;
  maxRssi = -200;
  rssiSum = 0;
}

} // namespace MeshCore

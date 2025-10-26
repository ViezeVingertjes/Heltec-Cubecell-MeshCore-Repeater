#include "TraceHandler.h"
#include "../../core/NodeConfig.h"
#include "../../core/PacketDecoder.h"
#include "../../radio/LoRaTransmitter.h"
#include <Arduino.h>
#include <string.h>

namespace MeshCore {

ProcessResult TraceHandler::processPacket(const PacketEvent &event,
                                          ProcessingContext &ctx) {
  if (event.packet.payloadType != PayloadType::TRACE) {
    return ProcessResult::CONTINUE;
  }

  if (event.packet.routeType != RouteType::DIRECT) {
    LOG_WARN("TRACE packet with non-DIRECT routing, dropping");
    return ProcessResult::DROP;
  }

  if (event.packet.payloadLength < MeshCore::TRACE_MIN_PAYLOAD_SIZE) {
    LOG_WARN("TRACE packet too small, dropping");
    return ProcessResult::DROP;
  }

  uint32_t traceTag;
  uint32_t authCode;
  uint8_t flags;
  memcpy(&traceTag, &event.packet.payload[0], MeshCore::TRACE_TAG_SIZE);
  memcpy(&authCode, &event.packet.payload[MeshCore::TRACE_TAG_SIZE], MeshCore::TRACE_AUTH_SIZE);
  flags = event.packet.payload[MeshCore::TRACE_TAG_SIZE + MeshCore::TRACE_AUTH_SIZE];

  uint8_t pathHashesLen = event.packet.payloadLength - MeshCore::TRACE_MIN_PAYLOAD_SIZE;
  const uint8_t *pathHashes = &event.packet.payload[MeshCore::TRACE_MIN_PAYLOAD_SIZE];

  uint8_t ourHash = NodeConfig::getInstance().getNodeHash();

  LOG_INFO_FMT("TRACE packet: tag=0x%08lX, auth=0x%08lX, flags=0x%02X, "
               "path_len=%d, snr_len=%d, our_hash=0x%02X",
               traceTag, authCode, flags, pathHashesLen,
               event.packet.pathLength, ourHash);

  if (event.packet.pathLength >= pathHashesLen) {
    handleTraceComplete(event.packet);
    tracesHandled++;
    return ProcessResult::STOP;
  }

  if (!shouldForwardTrace(event.packet, ctx)) {
    return ProcessResult::DROP;
  }

  uint8_t nextHopHash = pathHashes[event.packet.pathLength];
  if (!isNodeHashMatch(nextHopHash)) {
    LOG_INFO_FMT("TRACE not for us: next_hop=0x%02X, our_hash=0x%02X",
                 nextHopHash, ourHash);
    return ProcessResult::DROP;
  }

  LOG_INFO_FMT("TRACE for us! Appending SNR and forwarding (hop %d/%d)",
               event.packet.pathLength + 1, pathHashesLen);

  DecodedPacket forwardPacket;
  memcpy(&forwardPacket, &event.packet, sizeof(DecodedPacket));

  if (appendSnrAndForward(forwardPacket, event.snr)) {
    tracesHandled++;
    ctx.shouldForward = true;
  }

  return ProcessResult::STOP;
}

bool TraceHandler::isNodeHashMatch(uint8_t hash) const {
  uint8_t ourHash = NodeConfig::getInstance().getNodeHash();
  return ourHash == hash;
}

bool TraceHandler::shouldForwardTrace(const DecodedPacket &packet,
                                      const ProcessingContext &ctx) const {
  if (ctx.isDuplicate) {
    LOG_DEBUG("TRACE is duplicate, not forwarding");
    return false;
  }

  if (!Config::Forwarding::ENABLED) {
    LOG_DEBUG("Forwarding disabled, not forwarding TRACE");
    return false;
  }

  return true;
}

bool TraceHandler::appendSnrAndForward(DecodedPacket &packet, int8_t snr) {
  if (packet.pathLength >= MAX_PATH_SIZE) {
    LOG_ERROR("TRACE path full, cannot append SNR");
    return false;
  }

  int8_t snrScaled = snr;
  packet.path[packet.pathLength] = static_cast<uint8_t>(snrScaled);
  packet.pathLength++;

  int32_t snrDb = snr / 4;
  LOG_INFO_FMT("Appended SNR=%d dB to TRACE path (new path_len=%d)", snrDb,
               packet.pathLength);

  uint8_t rawPacket[256];
  uint16_t length = PacketDecoder::encode(packet, rawPacket, sizeof(rawPacket));

  if (length == 0) {
    LOG_ERROR("Failed to encode TRACE packet");
    return false;
  }

  LoRaTransmitter &transmitter = LoRaTransmitter::getInstance();

  if (transmitter.isTransmitting()) {
    LOG_WARN("Transmitter busy, cannot forward TRACE");
    return false;
  }

  if (!transmitter.canTransmitNow()) {
    LOG_WARN("In silence period, cannot forward TRACE");
    return false;
  }

  // TRACE packets use DIRECT routing, so collision risk is lower than FLOOD
  // No artificial delay needed - transmit immediately for lowest latency
  if (transmitter.transmit(rawPacket, length)) {
    LOG_INFO("TRACE packet forwarded");
    return true;
  }

  LOG_ERROR("Failed to transmit TRACE packet");
  return false;
}

void TraceHandler::handleTraceComplete(const DecodedPacket &packet) {
  uint32_t traceTag;
  memcpy(&traceTag, &packet.payload[0], MeshCore::TRACE_TAG_SIZE);

  LOG_INFO_FMT("TRACE complete! tag=0x%08lX, total hops=%d", traceTag,
               packet.pathLength);

  LOG_INFO("TRACE SNR values:");
  for (uint8_t i = 0; i < packet.pathLength; i++) {
    int8_t snrScaled = static_cast<int8_t>(packet.path[i]);
    int32_t snrDb = snrScaled / 4;
    LOG_INFO_FMT("  Hop %d: SNR=%d dB (raw=0x%02X)", i + 1, snrDb,
                 packet.path[i]);
  }
}

} // namespace MeshCore

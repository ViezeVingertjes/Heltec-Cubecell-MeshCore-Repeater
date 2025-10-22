#pragma once

#include "PacketDecoder.h"
#include "Result.h"
#include <Arduino.h>

namespace MeshCore {

/**
 * Validation levels for packet checking
 */
enum class ValidationLevel : uint8_t {
  BASIC = 0,   // Quick checks (bounds only)
  STANDARD = 1, // Standard checks (bounds + type validation)
  STRICT = 2    // Full validation (all fields + consistency)
};

/**
 * PacketValidator provides memory safety and data integrity checks
 * for all packet operations. Use this before processing untrusted data.
 */
class PacketValidator {
public:
  /**
   * Validate a decoded packet structure
   * 
   * @param packet Packet to validate
   * @param level Validation level (defaults to STANDARD)
   * @return Result with OK or specific error code
   */
  static Result<void> validate(const DecodedPacket &packet,
                                ValidationLevel level = ValidationLevel::STANDARD) {
    // Basic bounds checks (always performed)
    if (packet.pathLength > MAX_PATH_SIZE) {
      return Err(ErrorCode::PATH_TOO_LONG);
    }

    if (packet.payloadLength > MAX_PACKET_PAYLOAD) {
      return Err(ErrorCode::PAYLOAD_TOO_LARGE);
    }

    // Standard validation
    if (level >= ValidationLevel::STANDARD) {
      // Validate route type
      uint8_t routeTypeValue = static_cast<uint8_t>(packet.routeType);
      if (routeTypeValue > 0x03) {
        return Err(ErrorCode::INVALID_PACKET);
      }

      // Validate payload type
      uint8_t payloadTypeValue = static_cast<uint8_t>(packet.payloadType);
      if (payloadTypeValue > 0x0F) {
        return Err(ErrorCode::INVALID_PACKET);
      }

      // Validate transport codes if present
      if (packet.hasTransportCodes) {
        if (packet.transportCodes[0] == 0 && packet.transportCodes[1] == 0) {
          // Both zero is suspicious but not necessarily invalid
        }
      }
    }

    // Strict validation
    if (level >= ValidationLevel::STRICT) {
      // Validate advert data if decoded
      if (packet.isAdvertDecoded) {
        uint8_t advertTypeValue = static_cast<uint8_t>(packet.advertType);
        if (advertTypeValue > 4) {
          return Err(ErrorCode::INVALID_PACKET);
        }

        // Validate location if present
        if (packet.hasLocation) {
          if (packet.latitude < -90.0 || packet.latitude > 90.0) {
            return Err(ErrorCode::INVALID_PACKET);
          }
          if (packet.longitude < -180.0 || packet.longitude > 180.0) {
            return Err(ErrorCode::INVALID_PACKET);
          }
        }
      }

      // Validate payload version
      if (packet.payloadVersion > 3) {
        // Future versions might be valid, so just warn internally
      }
    }

    return Ok();
  }

  /**
   * Validate path array before modification
   */
  static Result<void> validatePath(const uint8_t *path, uint8_t length) {
    if (path == nullptr) {
      return Err(ErrorCode::INVALID_PARAMETER);
    }

    if (length > MAX_PATH_SIZE) {
      return Err(ErrorCode::PATH_TOO_LONG);
    }

    return Ok();
  }

  /**
   * Validate payload array
   */
  static Result<void> validatePayload(const uint8_t *payload, uint16_t length) {
    if (payload == nullptr && length > 0) {
      return Err(ErrorCode::INVALID_PARAMETER);
    }

    if (length > MAX_PACKET_PAYLOAD) {
      return Err(ErrorCode::PAYLOAD_TOO_LARGE);
    }

    return Ok();
  }

  /**
   * Check if path can accept one more node
   */
  static Result<void> canAddToPath(const DecodedPacket &packet) {
    if (packet.pathLength >= MAX_PATH_SIZE) {
      return Err(ErrorCode::PATH_TOO_LONG);
    }

    return Ok();
  }

  /**
   * Validate raw packet buffer before decoding
   */
  static Result<void> validateRawPacket(const uint8_t *raw, uint16_t length) {
    if (raw == nullptr) {
      return Err(ErrorCode::INVALID_PARAMETER);
    }

    if (length == 0) {
      return Err(ErrorCode::INVALID_PACKET);
    }

    // Minimum packet is 1 byte header + some payload
    if (length < 2) {
      return Err(ErrorCode::INVALID_PACKET);
    }

    // Maximum LoRa packet size check (typical limit is 255)
    if (length > 255) {
      return Err(ErrorCode::BUFFER_TOO_SMALL);
    }

    return Ok();
  }

  /**
   * Validate buffer has sufficient space
   */
  static Result<void> validateBufferSize(uint16_t required, uint16_t available) {
    if (required > available) {
      return Err(ErrorCode::BUFFER_TOO_SMALL);
    }

    return Ok();
  }

  /**
   * Check if node hash is already in path (loop detection)
   */
  static bool isNodeInPath(const DecodedPacket &packet, uint8_t nodeHash) {
    for (uint8_t i = 0; i < packet.pathLength; ++i) {
      if (packet.path[i] == nodeHash) {
        return true;
      }
    }
    return false;
  }

  /**
   * Validate RSSI value is in reasonable range
   */
  static Result<void> validateRSSI(int16_t rssi) {
    // LoRa typical range: -140 to 0 dBm
    if (rssi < -140 || rssi > 0) {
      return Err(ErrorCode::INVALID_PARAMETER);
    }

    return Ok();
  }

  /**
   * Validate SNR value is in reasonable range
   */
  static Result<void> validateSNR(int8_t snr) {
    // LoRa typical SNR range (in 0.25 dB units): -128 to +127
    // This represents -32 dB to +31.75 dB
    // No specific validation needed, all values are technically valid
    return Ok();
  }
};

} // namespace MeshCore


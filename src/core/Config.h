#pragma once

#include <Arduino.h>

// Firmware version
#define FIRMWARE_VERSION "1.0.0"
#define FIRMWARE_BUILD_DATE __DATE__

namespace Config {

namespace Power {
constexpr bool LIGHT_SLEEP_ENABLED = false;  // Disabled to test RX sensitivity
// Note: VEXT control is handled automatically by CubeCell framework
// No manual control is needed for HTCC-AB02A
} // namespace Power

namespace LoRa {
constexpr uint32_t FREQUENCY = 869618000;
constexpr uint8_t BANDWIDTH = 3;  // LORA_BW_062 = 62.5 kHz
constexpr uint8_t SPREADING_FACTOR = 8;
constexpr uint8_t CODING_RATE = 4;
constexpr uint8_t PREAMBLE_LENGTH = 16;
constexpr uint8_t SYNC_WORD = 0x12;
constexpr bool FIXED_LENGTH_PAYLOAD = false;
constexpr bool IQ_INVERSION = false;
constexpr uint8_t TX_POWER = 22;  // Maximum power for SX1262
constexpr uint32_t TX_TIMEOUT_MS = 3000;
} // namespace LoRa

namespace Logging {
constexpr size_t BUFFER_SIZE = 128;  // Reduced from 256 to save stack space
}

namespace Dispatcher {
constexpr size_t MAX_PROCESSORS = 8;
}

namespace Deduplication {
constexpr size_t CACHE_SIZE = 16;
constexpr uint32_t CACHE_TIMEOUT_MS = 60000;
} // namespace Deduplication

namespace Queue {
constexpr size_t PACKET_QUEUE_SIZE = 16;
}

namespace Channels {
// Private channel keys (hex format, 32 characters = 16 bytes)
// Add your private channel keys here
constexpr const char* PRIVATE_CHANNEL_KEYS[] = {
  "b4a28381f505eb67e696ed3d2294c81f",
  // Add more private channels here as needed
  // "1234567890abcdef1234567890abcdef",
};
constexpr size_t NUM_PRIVATE_CHANNELS = sizeof(PRIVATE_CHANNEL_KEYS) / sizeof(PRIVATE_CHANNEL_KEYS[0]);
} // namespace Channels

namespace Identity {
// Node name prefix (appears before the node hash in status messages)
constexpr const char* NODE_NAME = "VieZe Rogue";

// Enable custom node ID (false = use hardware-generated ID from chip)
constexpr bool USE_CUSTOM_NODE_ID = false;
constexpr uint16_t CUSTOM_NODE_ID = 0x0000;  // Range: 0x0001-0xFFFE

// Enable custom node hash (false = use hardware-generated hash from chip)
constexpr bool USE_CUSTOM_NODE_HASH = false;
constexpr uint8_t CUSTOM_NODE_HASH = 0x00;   // Range: 0x01-0xFE

// Location (included in advert packets)
// Set to true to include location in !advert responses
constexpr bool HAS_LOCATION = false;
constexpr int32_t LOCATION_LATITUDE = 51997698;   // 51.997698 in microdegrees
constexpr int32_t LOCATION_LONGITUDE = 5078354;   // 5.078354 in microdegrees
} // namespace Identity

namespace Forwarding {
constexpr bool ENABLED = true;
constexpr uint8_t MAX_PATH_LENGTH = 64;
constexpr int16_t MIN_RSSI_TO_FORWARD = -120;

// Delay calculation parameters - tune these for latency vs collision tradeoff
constexpr float RX_DELAY_BASE = 2.5f;           // Base for exponential backoff
constexpr float TX_DELAY_FACTOR = 2.0f;         // Jitter slot size multiplier

// WARNING: Duty cycle enforcement is currently disabled. Users MUST ensure
// compliance with local regulations (e.g., EU 868MHz requires 1% duty cycle).
// Consider implementing duty cycle tracking for production deployments.
constexpr bool DUTY_CYCLE_ENABLED = false;
constexpr uint32_t MIN_DELAY_THRESHOLD_MS = 20; // Reduced from 50ms for lower latency
constexpr uint8_t TX_DELAY_JITTER_SLOTS = 6;    // Random jitter slots (0-5)

// SNR-based packet scoring parameters
constexpr float SNR_SCALE_FACTOR = 4.0f;  // LoRa reports SNR in 0.25 dB units
constexpr float SNR_MIN_DB = -20.0f;      // Minimum expected SNR in dB
constexpr float SNR_RANGE_DB = 40.0f;     // Expected SNR range (from -20 to +20 dB)

// Delayed forwarding queue
constexpr size_t DELAY_QUEUE_SIZE = 4;

// Buffer sizes
constexpr size_t MAX_ENCODED_PACKET_SIZE = 256;
} // namespace Forwarding

} // namespace Config

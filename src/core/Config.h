#pragma once

#include <Arduino.h>

namespace Config {

namespace Power {
constexpr bool LIGHT_SLEEP_ENABLED = true;
constexpr bool DYNAMIC_VEXT_CONTROL = true;
} // namespace Power

namespace LED {
constexpr bool ENABLED = false;
constexpr uint8_t BRIGHTNESS = 25;
constexpr uint32_t FLASH_DURATION_MS = 50;
} // namespace LED

namespace LoRa {
constexpr uint32_t FREQUENCY = 869618000;
constexpr uint8_t BANDWIDTH = 3;
constexpr uint8_t SPREADING_FACTOR = 8;
constexpr uint8_t CODING_RATE = 4;
constexpr uint8_t PREAMBLE_LENGTH = 16;
constexpr uint8_t SYNC_WORD = 0x12;
constexpr bool FIXED_LENGTH_PAYLOAD = false;
constexpr bool IQ_INVERSION = false;
constexpr uint8_t TX_POWER = 21;
constexpr uint32_t TX_TIMEOUT_MS = 3000;
} // namespace LoRa

namespace Logging {
constexpr size_t BUFFER_SIZE = 128;  // Reduced from 256 to save stack space
}

namespace NodeIdentity {
// Set USE_MANUAL_ID to true to override hardware-generated ID
constexpr bool USE_MANUAL_ID = false;
constexpr uint16_t MANUAL_NODE_ID = 0xFF00;    // Custom test node ID
constexpr uint8_t MANUAL_NODE_HASH = 0x00;     // Reserved value - minimal collision risk with crypto-generated IDs
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

namespace Forwarding {
constexpr bool ENABLED = true;
constexpr uint8_t MAX_PATH_LENGTH = 64;
constexpr int16_t MIN_RSSI_TO_FORWARD = -120;

// Delay calculation parameters - tune these for latency vs collision tradeoff
constexpr float RX_DELAY_BASE = 2.5f;           // Base for exponential backoff
constexpr float TX_DELAY_FACTOR = 2.0f;         // Jitter slot size multiplier
constexpr float AIRTIME_BUDGET_FACTOR = 0.0f;   // Duty cycle limiting (0 = disabled)
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

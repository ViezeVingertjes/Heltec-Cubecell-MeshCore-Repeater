#pragma once

#include <Arduino.h>

namespace Config {

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
}

namespace Logging {
    constexpr size_t BUFFER_SIZE = 256;
}

namespace Dispatcher {
    constexpr size_t MAX_PROCESSORS = 8;
}

namespace Deduplication {
    constexpr size_t CACHE_SIZE = 16;
    constexpr uint32_t CACHE_TIMEOUT_MS = 60000;
}

namespace Queue {
    constexpr size_t PACKET_QUEUE_SIZE = 16;
}

namespace Forwarding {
    constexpr bool ENABLED = true;
    constexpr uint8_t MAX_PATH_LENGTH = 64;
    constexpr int16_t MIN_RSSI_TO_FORWARD = -120;
    
    constexpr float RX_DELAY_BASE = 2.5f;
    constexpr float TX_DELAY_FACTOR = 2.0f;
    constexpr float AIRTIME_BUDGET_FACTOR = 2.0f;
    constexpr uint32_t MIN_DELAY_THRESHOLD_MS = 50;
    constexpr uint8_t TX_DELAY_JITTER_SLOTS = 6;
}

}


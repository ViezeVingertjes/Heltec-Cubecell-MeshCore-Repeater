#pragma once

#include "core/constants.h"
#include <cstdint>
#include <cstddef>

namespace MiniCore::Config {
using namespace Constants;

constexpr uint32_t RADIO_FREQUENCY_HZ = 869431000;
constexpr uint8_t RADIO_SPREADING_FACTOR = 7;
constexpr uint8_t RADIO_CODING_RATE = 5;
constexpr uint8_t RADIO_PREAMBLE_LENGTH = 16;
constexpr int8_t RADIO_TX_POWER_DBM = 22;
constexpr uint16_t RADIO_SYNC_WORD = 0x12;
constexpr bool RADIO_CRC_ENABLED = true;
constexpr bool RADIO_IQ_INVERTED = false;

constexpr uint8_t CS_MAX_RETRIES = 8;
constexpr uint32_t CS_RETRY_DELAY_MS = 100;

constexpr float NOISE_FLOOR_EWMA_ALPHA = 0.1f;
constexpr float NOISE_FLOOR_VARIANCE_ALPHA = 0.1f;
constexpr float NOISE_FLOOR_SIGMA_MULTIPLIER = 3.0f;
constexpr uint8_t NOISE_FLOOR_MIN_SAMPLES = 10;
constexpr int8_t NOISE_FLOOR_MIN_MARGIN_DB = 6;

constexpr bool POWER_SAVE_ENABLED = false;
constexpr bool DROP_REPEATER_ADVERTS = false;
constexpr uint32_t POWER_SAVE_IDLE_TIMEOUT_MS = 60000;
constexpr uint32_t POWER_SAVE_SLEEP_DURATION_MS = 30000;

constexpr uint8_t DEFAULT_MAX_FLOOD_PATH = 64;
constexpr uint8_t REPEATER_SLOT_COUNT = 7;
constexpr uint32_t REPEATER_SLOT_AIRTIME_MS = 300;

constexpr uint8_t TX_QUEUE_SIZE = 16;
constexpr size_t FLASH_BUFFER_SIZE = 128;

constexpr uint8_t TIME_SYNC_MAX_SENDERS = 16;
constexpr uint8_t TIME_SYNC_MIN_CONSENSUS = 8;

constexpr uint32_t SERIAL_BAUD_RATE = 115200;
constexpr uint32_t STARTUP_DELAY_MS = 100;

constexpr size_t NODE_NAME_BUF_SIZE = 16;
constexpr size_t LOG_LINE_MAX_SIZE = 128;

constexpr uint32_t DISCOVER_RESP_DELAY_BASE_MS = 100;
constexpr uint32_t DISCOVER_RESP_DELAY_SPREAD_MS = 400;
constexpr uint32_t NOISE_FLOOR_SAMPLE_INTERVAL_MS = 500;
constexpr uint32_t NOISE_FLOOR_LOG_INTERVAL_MS = 60000;
constexpr uint32_t ADVERT_ENQUEUE_DELAY_MAX_MS = 1000;
constexpr uint32_t ADVERT_INTERVAL_HOURS = 12;

/** Default path hash mode = PATH_HASH_MODE_MAX (1 byte stored). Matches MeshCore path.hash.mode. */
constexpr uint8_t DEFAULT_PATH_HASH_MODE = PATH_HASH_MODE_MAX;

/** Flash offset for path hash mode (after identity + advert state). */
constexpr size_t PATH_HASH_MODE_STORAGE_OFFSET =
    IDENTITY_STORAGE_SIZE + ADVERT_STATE_STORAGE_SIZE;
constexpr size_t TOTAL_STORAGE_SIZE =
    PATH_HASH_MODE_STORAGE_OFFSET + PATH_HASH_MODE_STORAGE_SIZE;
}

#pragma once

#include <cstdint>
#include <cstddef>

namespace MiniCore::Constants {

constexpr size_t MAX_PACKET_PAYLOAD = 184;
constexpr size_t MAX_PATH_SIZE = 64;
constexpr size_t MAX_MTU_SIZE = 255;
constexpr size_t MIN_PACKET_SIZE = 3;
constexpr uint8_t MAX_HOPS = 32;

constexpr uint8_t MAX_HASH_SIZE = 8;
constexpr uint16_t MAX_PACKET_HASHES = 128;
constexpr uint8_t MAX_PACKET_ACKS = 64;

constexpr size_t ADVERT_PUBKEY_SIZE = 32;
constexpr size_t ADVERT_TIMESTAMP_OFFSET = 32;
constexpr size_t ADVERT_TIMESTAMP_SIZE = 4;
constexpr size_t ADVERT_SIGNATURE_SIZE = 64;
constexpr size_t ADVERT_MIN_SIZE = ADVERT_PUBKEY_SIZE + ADVERT_TIMESTAMP_SIZE + ADVERT_SIGNATURE_SIZE;
constexpr size_t MAX_ADVERT_DATA_SIZE = 32;
constexpr size_t SIGNATURE_SIZE = 64;
constexpr size_t ADVERT_STATE_STORAGE_SIZE = 4;

constexpr size_t SEED_SIZE = 32;
constexpr size_t PUB_KEY_SIZE = 32;
constexpr size_t PRV_KEY_SIZE = 64;
constexpr size_t IDENTITY_STORAGE_SIZE = PRV_KEY_SIZE + PUB_KEY_SIZE;

constexpr uint8_t PRIORITY_DIRECT = 0;
constexpr uint8_t PRIORITY_FLOOD_DEFAULT = 1;
constexpr uint8_t PRIORITY_PATH = 2;
constexpr uint8_t PRIORITY_ADVERT = 3;

constexpr uint8_t CTL_TYPE_NODE_DISCOVER_REQ = 0x80;
constexpr uint8_t CTL_TYPE_NODE_DISCOVER_RESP = 0x90;
constexpr uint8_t CTL_FLAG_PREFIX_ONLY = 0x01;
constexpr size_t DISCOVER_REQ_MIN_SIZE = 6;

constexpr uint8_t ADV_TYPE_CHAT_CLIENT = 0x01;
constexpr uint8_t ADV_TYPE_REPEATER = 0x02;
constexpr uint8_t ADV_TYPE_ROOM = 0x03;
constexpr uint8_t ADV_NAME_MASK = 0x80;
}

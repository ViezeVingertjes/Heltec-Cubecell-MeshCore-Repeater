#pragma once
#include <cstdint>
#include <cstddef>
#include <cstring>
#include "core/config.h"
#include "core/result.h"
#include "crypto/crypto_types.h"
namespace MiniCore {
using Config::CTL_TYPE_NODE_DISCOVER_REQ;
using Config::CTL_TYPE_NODE_DISCOVER_RESP;
using Config::CTL_FLAG_PREFIX_ONLY;
using Config::ADV_TYPE_REPEATER;
using Config::DISCOVER_REQ_MIN_SIZE;

constexpr size_t DISCOVER_REQ_FILTER_OFFSET = 1;
constexpr size_t DISCOVER_REQ_TAG_OFFSET = 2;
struct DiscoverRequest {
    uint8_t filter;
    uint32_t tag;
    bool prefixOnly;
};
[[nodiscard]] inline Status parseDiscoverRequest(const uint8_t* payload, uint16_t len, DiscoverRequest& req) {
    if (payload == nullptr) {
        return ErrorCode::InvalidParameter;
    }
    if (len < DISCOVER_REQ_MIN_SIZE) {
        return ErrorCode::InvalidParameter;
    }
    if ((payload[0] & 0xF0) != CTL_TYPE_NODE_DISCOVER_REQ) {
        return ErrorCode::InvalidParameter;
    }
    req.prefixOnly = (payload[0] & CTL_FLAG_PREFIX_ONLY) != 0;
    req.filter = payload[DISCOVER_REQ_FILTER_OFFSET];
    std::memcpy(&req.tag, &payload[DISCOVER_REQ_TAG_OFFSET], sizeof(req.tag));
    return Status();
}
[[nodiscard]] inline bool matchesNodeType(const DiscoverRequest& req, uint8_t nodeType) {
    return (req.filter & (1 << nodeType)) != 0;
}
constexpr size_t DISCOVER_RESP_MAX_SIZE = 64;
[[nodiscard]] inline Status createDiscoverResponse(
    const LocalIdentity& identity,
    uint32_t tag,
    int8_t snr,
    bool prefixOnly,
    uint8_t* buffer,
    uint8_t& len
) {
    if (buffer == nullptr) {
        return ErrorCode::InvalidParameter;
    }
    constexpr uint8_t PREFIX_SIZE = 8;
    uint8_t keyLen = prefixOnly ? PREFIX_SIZE : PUB_KEY_SIZE;
    len = 6 + keyLen;
    buffer[0] = CTL_TYPE_NODE_DISCOVER_RESP | ADV_TYPE_REPEATER;
    buffer[1] = static_cast<uint8_t>(snr);
    std::memcpy(&buffer[2], &tag, sizeof(tag));
    std::memcpy(&buffer[6], identity.publicKey.bytes, keyLen);
    return Status();
}
}

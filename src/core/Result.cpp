#include "Result.h"

namespace MeshCore {

const char *errorCodeToString(ErrorCode code) {
  switch (code) {
  case ErrorCode::OK:
    return "OK";
  case ErrorCode::QUEUE_FULL:
    return "Queue full";
  case ErrorCode::INVALID_PACKET:
    return "Invalid packet";
  case ErrorCode::TRANSMIT_FAILED:
    return "Transmit failed";
  case ErrorCode::BUFFER_TOO_SMALL:
    return "Buffer too small";
  case ErrorCode::PATH_TOO_LONG:
    return "Path too long";
  case ErrorCode::PAYLOAD_TOO_LARGE:
    return "Payload too large";
  case ErrorCode::DECODE_ERROR:
    return "Decode error";
  case ErrorCode::ENCODE_ERROR:
    return "Encode error";
  case ErrorCode::INVALID_PARAMETER:
    return "Invalid parameter";
  case ErrorCode::TIMEOUT:
    return "Timeout";
  case ErrorCode::HARDWARE_ERROR:
    return "Hardware error";
  case ErrorCode::DUPLICATE:
    return "Duplicate";
  case ErrorCode::WEAK_SIGNAL:
    return "Weak signal";
  case ErrorCode::ALREADY_EXISTS:
    return "Already exists";
  case ErrorCode::NOT_FOUND:
    return "Not found";
  default:
    return "Unknown error";
  }
}

} // namespace MeshCore


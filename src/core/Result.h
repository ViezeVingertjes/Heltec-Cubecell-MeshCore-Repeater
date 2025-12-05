#pragma once

#include <Arduino.h>

namespace MeshCore {

/**
 * Error codes for consistent error handling across the firmware
 */
enum class ErrorCode : uint8_t {
  OK = 0,
  QUEUE_FULL = 1,
  INVALID_PACKET = 2,
  TRANSMIT_FAILED = 3,
  BUFFER_TOO_SMALL = 4,
  PATH_TOO_LONG = 5,
  PAYLOAD_TOO_LARGE = 6,
  DECODE_ERROR = 7,
  ENCODE_ERROR = 8,
  INVALID_PARAMETER = 9,
  TIMEOUT = 10,
  HARDWARE_ERROR = 11,
  DUPLICATE = 12,
  WEAK_SIGNAL = 13,
  ALREADY_EXISTS = 14,
  NOT_FOUND = 15,
  UNKNOWN_ERROR = 255
};

/**
 * Result type for operations that can fail
 * Provides type-safe error handling with explicit success/failure
 * 
 * @tparam T Value type on success
 */
template <typename T> struct Result {
  T value;
  ErrorCode error;

  Result() : value{}, error(ErrorCode::OK) {}
  Result(const T &val) : value(val), error(ErrorCode::OK) {}
  Result(ErrorCode err) : value{}, error(err) {}
  Result(const T &val, ErrorCode err) : value(val), error(err) {}

  /**
   * Check if operation succeeded
   */
  bool isOk() const { return error == ErrorCode::OK; }

  /**
   * Check if operation failed
   */
  bool isError() const { return error != ErrorCode::OK; }

  /**
   * Get value or default if error
   */
  T valueOr(const T &defaultValue) const {
    return isOk() ? value : defaultValue;
  }

  /**
   * Implicit conversion to bool (true if OK)
   */
  explicit operator bool() const { return isOk(); }
};

/**
 * Specialized Result for operations that don't return a value
 */
template <> struct Result<void> {
  ErrorCode error;

  Result() : error(ErrorCode::OK) {}
  Result(ErrorCode err) : error(err) {}

  bool isOk() const { return error == ErrorCode::OK; }
  bool isError() const { return error != ErrorCode::OK; }
  explicit operator bool() const { return isOk(); }
};

/**
 * Convert error code to human-readable string
 */
const char *errorCodeToString(ErrorCode code);

/**
 * Helper to create success result
 */
template <typename T> Result<T> Ok(const T &value) {
  return Result<T>(value, ErrorCode::OK);
}

/**
 * Helper to create success result (void)
 */
inline Result<void> Ok() { return Result<void>(ErrorCode::OK); }

/**
 * Helper to create error result
 */
template <typename T> Result<T> Err(ErrorCode error) {
  return Result<T>(error);
}

/**
 * Helper to create error result (void)
 */
inline Result<void> Err(ErrorCode error) { return Result<void>(error); }

} // namespace MeshCore


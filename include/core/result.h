#pragma once

#include <cstdint>
#include <type_traits>
#include <utility>

namespace MiniCore {
enum class ErrorCode : uint8_t {
    None = 0,
    InvalidParameter,
    BufferOverflow,
    BufferTooSmall,
    Timeout,
    NotInitialized,
    AlreadyInitialized,
    HardwareError,
    InvalidState,
    StorageError,
    CryptoError,
    NotFound,
    Busy,
    TooManyHops,
    QueueFull
};
inline const char* errorCodeToString(ErrorCode code) {
    switch (code) {
        case ErrorCode::None: return "None";
        case ErrorCode::InvalidParameter: return "InvalidParameter";
        case ErrorCode::BufferOverflow: return "BufferOverflow";
        case ErrorCode::BufferTooSmall: return "BufferTooSmall";
        case ErrorCode::Timeout: return "Timeout";
        case ErrorCode::NotInitialized: return "NotInitialized";
        case ErrorCode::AlreadyInitialized: return "AlreadyInitialized";
        case ErrorCode::HardwareError: return "HardwareError";
        case ErrorCode::InvalidState: return "InvalidState";
        case ErrorCode::StorageError: return "StorageError";
        case ErrorCode::CryptoError: return "CryptoError";
        case ErrorCode::NotFound: return "NotFound";
        case ErrorCode::Busy: return "Busy";
        case ErrorCode::TooManyHops: return "TooManyHops";
        case ErrorCode::QueueFull: return "QueueFull";
        default: return "Unknown";
    }
}
template<typename T>
class Result {
public:
    constexpr Result(T value) : value_(value), error_(ErrorCode::None) {}
    constexpr Result(ErrorCode error) : value_{}, error_(error) {}
    [[nodiscard]] constexpr bool isOk() const { return error_ == ErrorCode::None; }
    [[nodiscard]] constexpr bool isError() const { return error_ != ErrorCode::None; }
    [[nodiscard]] constexpr ErrorCode error() const { return error_; }
    [[nodiscard]] constexpr const T& value() const { return value_; }
    [[nodiscard]] constexpr T& value() { return value_; }
        [[nodiscard]] constexpr T valueOr(const T& defaultValue) const {
        return isOk() ? value_ : defaultValue;
    }
    template<typename F>
    [[nodiscard]] auto map(F&& f) const -> Result<decltype(f(std::declval<const T&>()))> {
        using U = decltype(f(std::declval<const T&>()));
        if (isError()) {
            return Result<U>(error_);
        }
        return Result<U>(f(value_));
    }
    template<typename F>
    [[nodiscard]] auto and_then(F&& f) const {
        if (isError()) {
            return decltype(f(std::declval<const T&>()))(error_);
        }
        return f(value_);
    }
private:
    T value_;
    ErrorCode error_;
};
template<>
class Result<void> {
public:
    constexpr Result() : error_(ErrorCode::None) {}
    constexpr Result(ErrorCode error) : error_(error) {}
        [[nodiscard]] constexpr bool isOk() const { return error_ == ErrorCode::None; }
    [[nodiscard]] constexpr bool isError() const { return error_ != ErrorCode::None; }
    [[nodiscard]] constexpr ErrorCode error() const { return error_; }
    template<typename F>
    [[nodiscard]] Result<void> and_then(F&& f) const {
        if (isError()) {
            return Result<void>(error_);
        }
        return f();
    }
private:
    ErrorCode error_;
};

using Status = Result<void>;

}

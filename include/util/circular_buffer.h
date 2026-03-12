#pragma once
#include <cstddef>
#include <cstdint>
#include "core/result.h"
namespace MiniCore {
template<typename T, size_t Capacity>
class CircularBuffer {
public:
    static_assert(Capacity > 0, "Capacity must be greater than 0");
    static_assert(Capacity <= 255, "Capacity must fit in uint8_t");
    constexpr CircularBuffer() = default;
    [[nodiscard]] Result<void> push(const T& item);
    [[nodiscard]] Result<T> pop();
    [[nodiscard]] Result<T> peek() const;
    [[nodiscard]] constexpr uint8_t size() const { return count_; }
    [[nodiscard]] constexpr uint8_t capacity() const { return static_cast<uint8_t>(Capacity); }
    [[nodiscard]] constexpr bool isEmpty() const { return count_ == 0; }
    [[nodiscard]] constexpr bool isFull() const { return count_ == Capacity; }
    void clear();
private:
    T buffer_[Capacity]{};
    uint8_t head_{0};
    uint8_t tail_{0};
    uint8_t count_{0};
};
template<typename T, size_t Capacity>
Result<void> CircularBuffer<T, Capacity>::push(const T& item) {
    if (isFull()) {
        return ErrorCode::BufferOverflow;
    }
    buffer_[tail_] = item;
    tail_ = (tail_ + 1) % Capacity;
    ++count_;
    return {};
}
template<typename T, size_t Capacity>
Result<T> CircularBuffer<T, Capacity>::pop() {
    if (isEmpty()) {
        return ErrorCode::InvalidState;
    }
    T item = buffer_[head_];
    head_ = (head_ + 1) % Capacity;
    --count_;
    return item;
}
template<typename T, size_t Capacity>
Result<T> CircularBuffer<T, Capacity>::peek() const {
    if (isEmpty()) {
        return ErrorCode::InvalidState;
    }
    return buffer_[head_];
}
template<typename T, size_t Capacity>
void CircularBuffer<T, Capacity>::clear() {
    head_ = 0;
    tail_ = 0;
    count_ = 0;
}
}

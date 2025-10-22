#pragma once

#include <Arduino.h>

namespace MeshCore {

/**
 * Generic circular buffer for fixed-size collections with automatic wraparound.
 * Optimized for embedded systems with no dynamic allocation.
 * 
 * @tparam T Item type to store
 * @tparam SIZE Maximum number of items
 */
template <typename T, size_t SIZE> class CircularBuffer {
public:
  CircularBuffer() : writeIndex(0) {
    for (size_t i = 0; i < SIZE; ++i) {
      buffer[i] = T{};
    }
  }

  /**
   * Add item to the buffer, overwriting oldest if full
   */
  void push(const T &item) {
    buffer[writeIndex] = item;
    writeIndex = (writeIndex + 1) % SIZE;
  }

  /**
   * Get item at index (0 = oldest, SIZE-1 = newest)
   */
  const T &operator[](size_t index) const {
    return buffer[(writeIndex + index) % SIZE];
  }

  T &operator[](size_t index) { return buffer[(writeIndex + index) % SIZE]; }

  /**
   * Get item at absolute position in buffer
   */
  const T &at(size_t index) const { return buffer[index]; }

  T &at(size_t index) { return buffer[index]; }

  /**
   * Get maximum capacity
   */
  constexpr size_t capacity() const { return SIZE; }

  /**
   * Clear all items (sets to default value)
   */
  void clear() {
    for (size_t i = 0; i < SIZE; ++i) {
      buffer[i] = T{};
    }
    writeIndex = 0;
  }

  /**
   * Iterate over all items with custom function
   * 
   * @param fn Function called for each item: bool fn(T& item, size_t index)
   *           Return false to stop iteration early
   */
  template <typename Func> void forEach(Func fn) {
    for (size_t i = 0; i < SIZE; ++i) {
      if (!fn(buffer[i], i)) {
        break;
      }
    }
  }

  /**
   * Iterate over all items (const version)
   */
  template <typename Func> void forEach(Func fn) const {
    for (size_t i = 0; i < SIZE; ++i) {
      if (!fn(buffer[i], i)) {
        break;
      }
    }
  }

private:
  T buffer[SIZE];
  size_t writeIndex;
};

} // namespace MeshCore


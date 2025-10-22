#pragma once

#include <Arduino.h>
#include <string.h>

namespace MeshCore {

/**
 * Simple priority queue for fixed-size collections.
 * Items are kept sorted by a key field, smallest key first.
 * Optimized for embedded systems with no dynamic allocation.
 * 
 * @tparam T Item type (must have 'valid' bool field)
 * @tparam SIZE Maximum number of items
 * @tparam KeyType Type used for priority comparison
 */
template <typename T, size_t SIZE, typename KeyType = uint32_t>
class PriorityQueue {
public:
  PriorityQueue() : count(0) {
    for (size_t i = 0; i < SIZE; ++i) {
      items[i].valid = false;
    }
  }

  /**
   * Insert item in sorted order by key
   * 
   * @param item Item to insert
   * @param key Priority key (smaller = higher priority)
   * @return true if inserted, false if queue is full
   */
  bool insert(const T &item, KeyType key) {
    // Find free slot
    if (count >= SIZE) {
      return false;
    }

    // Find insertion position
    size_t insertIdx = count;
    for (size_t i = 0; i < count; ++i) {
      if (keys[i] > key) {
        insertIdx = i;
        break;
      }
    }

    // Shift items right to make space
    for (size_t i = count; i > insertIdx; --i) {
      items[i] = items[i - 1];
      keys[i] = keys[i - 1];
    }

    // Insert new item
    items[insertIdx] = item;
    keys[insertIdx] = key;
    count++;

    return true;
  }

  /**
   * Remove and return the highest priority (smallest key) item
   * 
   * @param out Output parameter for the item
   * @return true if item was retrieved, false if queue is empty
   */
  bool popFront(T &out) {
    if (count == 0) {
      return false;
    }

    out = items[0];

    // Shift remaining items left
    for (size_t i = 0; i < count - 1; ++i) {
      items[i] = items[i + 1];
      keys[i] = keys[i + 1];
    }

    items[count - 1].valid = false;
    count--;

    return true;
  }

  /**
   * Peek at the highest priority item without removing it
   */
  const T *peekFront() const {
    if (count == 0) {
      return nullptr;
    }
    return &items[0];
  }

  /**
   * Get the key of the front item
   */
  bool peekFrontKey(KeyType &outKey) const {
    if (count == 0) {
      return false;
    }
    outKey = keys[0];
    return true;
  }

  /**
   * Check if queue is empty
   */
  bool isEmpty() const { return count == 0; }

  /**
   * Check if queue is full
   */
  bool isFull() const { return count >= SIZE; }

  /**
   * Get current number of items
   */
  size_t size() const { return count; }

  /**
   * Get maximum capacity
   */
  constexpr size_t capacity() const { return SIZE; }

  /**
   * Clear all items
   */
  void clear() {
    for (size_t i = 0; i < SIZE; ++i) {
      items[i].valid = false;
    }
    count = 0;
  }

  /**
   * Remove items where predicate returns true
   * 
   * @param pred Predicate function: bool pred(const T& item, KeyType key)
   * @return Number of items removed
   */
  template <typename Pred> size_t removeIf(Pred pred) {
    size_t removed = 0;
    size_t writeIdx = 0;

    for (size_t readIdx = 0; readIdx < count; ++readIdx) {
      if (pred(items[readIdx], keys[readIdx])) {
        removed++;
      } else {
        if (writeIdx != readIdx) {
          items[writeIdx] = items[readIdx];
          keys[writeIdx] = keys[readIdx];
        }
        writeIdx++;
      }
    }

    count = writeIdx;
    for (size_t i = count; i < SIZE; ++i) {
      items[i].valid = false;
    }

    return removed;
  }

private:
  T items[SIZE];
  KeyType keys[SIZE];
  size_t count;
};

} // namespace MeshCore


#pragma once
#include <cstdint>
#include <cstddef>
#include "core/result.h"
namespace MiniCore {
class IStorage {
public:
    virtual ~IStorage() = default;
        virtual Status init(size_t size) = 0;
    virtual void deinit() = 0;
        virtual Result<uint8_t> read(size_t address) const = 0;
    virtual Status write(size_t address, uint8_t value) = 0;
        virtual Status readBlock(size_t address, uint8_t* buffer, size_t size) const = 0;
    virtual Status writeBlock(size_t address, const uint8_t* data, size_t size) = 0;
        virtual Status commit() = 0;
    virtual size_t capacity() const = 0;
};
}

#pragma once
#include <cstddef>
#include <cstdint>
#include "core/result.h"
namespace MiniCore {
class ISerial {
public:
    virtual ~ISerial() = default;
        virtual Status begin(uint32_t baudRate) = 0;
    virtual void end() = 0;
        virtual size_t write(uint8_t byte) = 0;
    virtual size_t write(const uint8_t* buffer, size_t size) = 0;
        virtual int available() = 0;
    virtual int read() = 0;
    virtual int peek() = 0;
    virtual void flush() = 0;
};
}

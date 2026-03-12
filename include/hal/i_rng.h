#pragma once
#include <cstdint>
#include <cstddef>
namespace MiniCore {
class IRng {
public:
    virtual ~IRng() = default;
        virtual void fill(uint8_t* buffer, size_t size) = 0;
    virtual uint32_t next() = 0;
};
}

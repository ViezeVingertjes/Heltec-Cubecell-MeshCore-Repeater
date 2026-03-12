#pragma once
#include <cstddef>
#include <cstdint>
namespace MiniCore {
inline void secureWipe(void* ptr, size_t len) {
    volatile uint8_t* p = static_cast<volatile uint8_t*>(ptr);
    while (len--) {
        *p++ = 0;
    }
}
}

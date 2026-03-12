#pragma once
#include <Arduino.h>
#include "hal/i_rng.h"
namespace MiniCore::TestHelpers {
class TestRng : public IRng {
public:
    void fill(uint8_t* buffer, size_t size) override {
        for (size_t i = 0; i < size; ++i) {
            buffer[i] = static_cast<uint8_t>(random(256));
        }
    }
    uint32_t next() override { 
        return static_cast<uint32_t>(random(0x7FFFFFFF)); 
    }
};
}

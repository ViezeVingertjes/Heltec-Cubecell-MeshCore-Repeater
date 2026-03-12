#pragma once
#include "hal/i_rng.h"
#include <vector>
#include <cstring>
namespace MiniCore::Mocks {
class MockRng : public IRng {
public:
    std::vector<uint8_t> presetData;
    size_t presetIndex{0};
    uint32_t nextValue{0};
    bool fillCalled{false};
    size_t fillSize{0};
        void fill(uint8_t* buffer, size_t size) override {
        fillCalled = true;
        fillSize = size;
                if (!presetData.empty()) {
            for (size_t i = 0; i < size; ++i) {
                buffer[i] = presetData[(presetIndex + i) % presetData.size()];
            }
            presetIndex = (presetIndex + size) % presetData.size();
        } else {
            std::memset(buffer, 0, size);
        }
    }
        uint32_t next() override {
        return nextValue;
    }
        void setPresetData(const std::vector<uint8_t>& data) {
        presetData = data;
        presetIndex = 0;
    }
        void reset() {
        presetData.clear();
        presetIndex = 0;
        nextValue = 0;
        fillCalled = false;
        fillSize = 0;
    }
};
}

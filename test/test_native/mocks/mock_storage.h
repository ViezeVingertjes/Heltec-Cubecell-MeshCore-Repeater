#pragma once
#include "hal/i_storage.h"
#include <vector>
namespace MiniCore::Mocks {
class MockStorage : public IStorage {
public:
    std::vector<uint8_t> data;
    bool initialized{false};
    bool commitCalled{false};
    size_t storageSize{0};
        Status initResult{};
    Status writeResult{};
    Status commitResult{};
        Status init(size_t size) override {
        if (initialized) return ErrorCode::AlreadyInitialized;
        storageSize = size;
        data.resize(size, 0xFF);
        initialized = true;
        return initResult;
    }
        void deinit() override {
        data.clear();
        initialized = false;
        storageSize = 0;
    }
        Result<uint8_t> read(size_t address) const override {
        if (!initialized) return ErrorCode::NotInitialized;
        if (address >= storageSize) return ErrorCode::InvalidParameter;
        return data[address];
    }
        Status write(size_t address, uint8_t value) override {
        if (!initialized) return ErrorCode::NotInitialized;
        if (address >= storageSize) return ErrorCode::InvalidParameter;
        data[address] = value;
        return writeResult;
    }
        Status readBlock(size_t address, uint8_t* buffer, size_t size) const override {
        if (!initialized) return ErrorCode::NotInitialized;
        if (address + size > storageSize) return ErrorCode::InvalidParameter;
        for (size_t i = 0; i < size; ++i) {
            buffer[i] = data[address + i];
        }
        return {};
    }
        Status writeBlock(size_t address, const uint8_t* buffer, size_t size) override {
        if (!initialized) return ErrorCode::NotInitialized;
        if (address + size > storageSize) return ErrorCode::InvalidParameter;
        for (size_t i = 0; i < size; ++i) {
            data[address + i] = buffer[i];
        }
        return writeResult;
    }
        Status commit() override {
        if (!initialized) return ErrorCode::NotInitialized;
        commitCalled = true;
        return commitResult;
    }
        size_t capacity() const override { return storageSize; }
        void reset() {
        deinit();
        commitCalled = false;
        initResult = {};
        writeResult = {};
        commitResult = {};
    }
};
}

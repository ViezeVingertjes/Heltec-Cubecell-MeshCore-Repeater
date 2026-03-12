#pragma once
#include "core/config.h"
#include "hal/i_storage.h"
namespace MiniCore {
class FlashStorage : public IStorage {
public:
    Status init(size_t size) override;
    void deinit() override;
        Result<uint8_t> read(size_t address) const override;
    Status write(size_t address, uint8_t value) override;
    Status readBlock(size_t address, uint8_t* buffer, size_t size) const override;
    Status writeBlock(size_t address, const uint8_t* data, size_t size) override;
        Status commit() override;
    size_t capacity() const override;
private:
    uint8_t buffer_[Config::FLASH_BUFFER_SIZE]{};
    size_t size_{0};
    bool dirty_{false};
};
}

#include "flash_storage.h"
#include <cstring>
#ifdef __asr650x__
#include <EEPROM.h>
#endif
namespace MiniCore {
Status FlashStorage::init(size_t size) {
    if (size > Config::FLASH_BUFFER_SIZE) {
        return ErrorCode::InvalidParameter;
    }
    size_ = size;
    #ifdef __asr650x__
    EEPROM.begin(size);
    for (size_t i = 0; i < size; ++i) {
        buffer_[i] = EEPROM.read(i);
    }
#else
    memset(buffer_, 0xFF, size);
#endif
        dirty_ = false;
    return Status();
}
void FlashStorage::deinit() {
    size_ = 0;
}
Result<uint8_t> FlashStorage::read(size_t address) const {
    if (address >= size_) {
        return ErrorCode::InvalidParameter;
    }
    return buffer_[address];
}
Status FlashStorage::write(size_t address, uint8_t value) {
    if (address >= size_) {
        return ErrorCode::InvalidParameter;
    }
    buffer_[address] = value;
    dirty_ = true;
    return Status();
}
Status FlashStorage::readBlock(size_t address, uint8_t* buffer, size_t size) const {
    if (address + size > size_) {
        return ErrorCode::InvalidParameter;
    }
    memcpy(buffer, &buffer_[address], size);
    return Status();
}
Status FlashStorage::writeBlock(size_t address, const uint8_t* data, size_t size) {
    if (address + size > size_) {
        return ErrorCode::InvalidParameter;
    }
    memcpy(&buffer_[address], data, size);
    dirty_ = true;
    return Status();
}
Status FlashStorage::commit() {
    if (!dirty_) {
        return Status();
    }
    #ifdef __asr650x__
    for (size_t i = 0; i < size_; ++i) {
        EEPROM.write(i, buffer_[i]);
    }
    if (!EEPROM.commit()) {
        return ErrorCode::HardwareError;
    }
#endif
        dirty_ = false;
    return Status();
}
size_t FlashStorage::capacity() const {
    return size_;
}
}

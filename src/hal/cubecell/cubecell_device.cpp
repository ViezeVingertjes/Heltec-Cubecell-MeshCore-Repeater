#include "cubecell_device.h"
#ifdef __asr650x__
#include <Arduino.h>
#include "innerWdt.h"
#include "LoRaWan_APP.h"
#include "core/config.h"
namespace MiniCore {
void CubeCellBoard::reset() {
    CySoftwareReset();
}
DeviceId CubeCellBoard::getUniqueId() const {
    DeviceId id{};
    uint32_t seed = getRandomSeed();
    for (size_t i = 0; i < sizeof(id.bytes); ++i) {
        id.bytes[i] = static_cast<uint8_t>((seed >> (i * 4)) & 0xFF);
    }
    return id;
}
uint32_t CubeCellBoard::getRandomSeed() const {
    return analogRead(0) ^ (millis() << 16);
}
void CubeCellBoard::disableInterrupts() {
    noInterrupts();
}
void CubeCellBoard::enableInterrupts() {
    interrupts();
}
void CubeCellBoard::feedWatchdog() {
    feedInnerWdt();
}
CubeCellDevice& CubeCellDevice::instance() {
    static CubeCellDevice inst;
    return inst;
}
Status CubeCellDevice::init() {
    pinMode(Vext, OUTPUT);
    digitalWrite(Vext, LOW);
        pinMode(USER_KEY, INPUT);
        innerWdtEnable(true);
        size_t storageSize = Config::TOTAL_STORAGE_SIZE;
    auto storageResult = storage_.init(storageSize);
    if (!storageResult.isOk()) {
        return storageResult;
    }
        randomSeed(analogRead(0));
        return Status();
}
void CubeCellDevice::processLoop() {
    Radio.IrqProcess();
}
void CubeCellDevice::idle() {
    __WFI();
}
Status CubeCellDevice::factoryReset() {
    size_t storageSize = Config::TOTAL_STORAGE_SIZE;
    uint8_t emptyData[Config::TOTAL_STORAGE_SIZE];
    memset(emptyData, 0xFF, sizeof(emptyData));
    auto writeResult = storage_.writeBlock(0, emptyData, storageSize);
    if (!writeResult.isOk()) {
        return writeResult;
    }
    return storage_.commit();
}
bool CubeCellDevice::checkFactoryResetRequest(uint32_t timeoutMs) {
    uint32_t start = millis();
    bool buttonHeld = false;
        while (millis() - start < timeoutMs) {
        if (digitalRead(USER_KEY) == LOW) {
            buttonHeld = true;
        } else if (buttonHeld) {
            return true;
        }
        delay(50);
    }
        return false;
}
void CubeCellDevice::reboot() {
    CySoftwareReset();
}
IDevice& createDevice() {
    return CubeCellDevice::instance();
}
}
#endif

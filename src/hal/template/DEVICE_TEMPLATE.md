# Device Implementation Template

This directory serves as a template for implementing new device support in MiniCore.

## Steps to Add a New Device

### 1. Create the HAL Directory

Create a new directory for your device:
```
src/hal/<device_name>/
```

For example: `src/hal/esp32/`, `src/hal/nrf52/`, `src/hal/rp2040/`

### 2. Implement Required HAL Interfaces

Copy and adapt these files from the cubecell implementation:

#### Required Files:

| File | Interface | Description |
|------|-----------|-------------|
| `<device>_device.h` | `IDevice` | Main device class combining all HAL components |
| `<device>_device.cpp` | `IDevice` | Implementation including factory function |
| `<device>_radio.h/.cpp` | `IRadio` | LoRa radio driver |
| `<device>_timer.h` | `ITimer` | Timing functions (millis, micros, delay) |
| `<device>_rng.h` | `IRng` | Random number generator |
| `<device>_log.h` | `ILog` | Logging/debug output |
| `<device>_storage.h/.cpp` | `IStorage` | Persistent storage (EEPROM/Flash) |

#### Optional Files:

| File | Interface | Description |
|------|-----------|-------------|
| `<device>_gpio.h` | `IGpio` | GPIO pin control |
| `<device>_power.h` | `IPower` | Power management, battery status |

### 3. Implement the Device Class

Your device class should:

```cpp
#pragma once

#ifdef <YOUR_DEVICE_MACRO>  // e.g., ESP32, NRF52840, etc.

#include "hal/i_device.h"
// Include your platform HAL headers

namespace MiniCore {

class YourBoard : public IBoard {
public:
    void reset() override;
    DeviceId getUniqueId() const override;
    uint32_t getRandomSeed() const override;
    void disableInterrupts() override;
    void enableInterrupts() override;
    void feedWatchdog() override;
};

class YourDevice : public IDevice {
public:
    static YourDevice& instance();
    
    Status init() override;
    const char* name() const override { return "YourDevice"; }
    
    IBoard& board() override { return board_; }
    ILog& log() override { return log_; }
    ITimer& timer() override { return timer_; }
    IRng& rng() override { return rng_; }
    IStorage& storage() override { return storage_; }
    IRadio& radio() override { return radio_; }
    
    void processLoop() override;
    void idle() override;
    void factoryReset() override;
    bool checkFactoryResetRequest(uint32_t timeoutMs) override;
    void reboot() override;

private:
    YourDevice() = default;
    
    YourBoard board_;
    YourLog log_;
    YourTimer timer_;
    YourRng rng_;
    YourStorage storage_;
    YourRadio radio_;
};

}

#endif
```

### 4. Implement the Device Factory

At the end of your `<device>_device.cpp`, include the factory function:

```cpp
MiniCore::IDevice& MiniCore::createDevice() {
    return MiniCore::YourDevice::instance();
}
```

### 5. Add PlatformIO Environment

Add to `platformio.ini`:

```ini
[env:your_device_board]
platform = <platform>
board = <board>
framework = arduino
monitor_speed = 115200
build_flags =
    ${common_embedded.build_flags}
    -D<YOUR_DEVICE_MACRO>
    -DNDEBUG
build_src_flags =
    ${common_embedded.build_src_flags}
build_unflags =
    ${common_embedded.build_unflags}
lib_deps =
    ; Add any required libraries
```

### 6. Key Implementation Notes

#### Radio Implementation

The `IRadio` interface is critical. Your implementation must:

1. Support LoRa modulation with configurable parameters
2. Handle RX/TX callbacks properly
3. Support instant RSSI reading for carrier sense (channel busy detection)

Example radio chips and their typical platforms:
- **SX1276/78**: CubeCell, many ESP32 boards
- **SX1262**: Newer Heltec boards, RAK boards
- **RFM95**: Adafruit Feather, generic modules

#### Timer Implementation

Most Arduino-compatible frameworks provide `millis()` and `micros()`:

```cpp
class YourTimer : public ITimer {
public:
    uint32_t millis() const override { return ::millis(); }
    uint32_t micros() const override { return ::micros(); }
    void delayMs(uint32_t ms) override { delay(ms); }
    void delayUs(uint32_t us) override { delayMicroseconds(us); }
};
```

#### Storage Implementation

Storage should provide at least 128 bytes of persistent storage:
- ESP32: Preferences or SPIFFS
- nRF52: Internal flash or NVMC
- RP2040: Flash with wear leveling

#### Factory Reset Button

Implement `checkFactoryResetRequest()` to detect a hardware button press:
- Hold for N seconds to trigger factory reset
- Return `true` if reset was requested

### 7. Testing Your Implementation

1. Build for your target:
   ```bash
   pio run -e your_device_board
   ```

2. Flash and monitor:
   ```bash
   pio run -e your_device_board -t upload
   pio device monitor -b 115200
   ```

3. Verify output shows your device name:
   ```
   MiniCore v1.0.0 [YourDevice]
   ```

## Reference Implementation

See `src/hal/cubecell/` for a complete working example.

## Common Preprocessor Macros by Platform

| Platform | Typical Macro |
|----------|---------------|
| Heltec CubeCell | `__asr650x__` |
| ESP32 | `ESP32` or `ESP_PLATFORM` |
| nRF52 | `NRF52` or `NRF52840_XXAA` |
| RP2040 | `ARDUINO_ARCH_RP2040` |
| STM32 | `STM32F1` etc. |
| Teensy | `TEENSYDUINO` |

# HTCC-AB02A Hardware Configuration

This document describes the hardware pins and features used by this firmware on the Heltec CubeCell HTCC-AB02A board.

## Board Information

- **Model**: Heltec CubeCell HTCC-AB02A
- **MCU**: ASR6501 (ARM Cortex-M0+ with LoRa)
- **LoRa Radio**: SX1262
- **Operating Voltage**: 3.3V
- **Battery**: 3.7V LiPo (via JST connector)

## Pin Usage Summary

This firmware uses **framework-abstracted functions** and does not directly reference GPIO pins. All hardware access goes through the CubeCell Arduino framework, which handles the board-specific pin mapping automatically.

### Pins Used (via Framework)

| Function | Framework API | HTCC-AB02A Hardware | Notes |
|----------|--------------|---------------------|-------|
| Battery Voltage | `getBatteryVoltage()` | ADC1 (GPIO3, pin 3) | Framework handles ADC reading |
| Entropy Source | `analogRead(ADC)` | ADC_USER_CHANNEL | Used for random number generation |
| LoRa Radio | `Radio.Init()` | SX1262 via SPI | Framework configures SPI pins automatically |
| Power Management | `lowPowerHandler()` | MCU sleep modes | Framework handles sleep/wake |
| VEXT Control | Automatic | GPIO1015 (pin 19) | Framework controls external power |

### Pins NOT Used

The following HTCC-AB02A pins are **available for future expansion**:

- **USER_BUTTON**: GPIO1016 (pin 20) - Available but not used
- **ADC2**: GPIO2 (pin 2) - Available
- **ADC3**: GPIO4 (pin 4) - Available  
- **PWM1/PWM2**: GPIO103/GPIO104 (pins 10/11) - Available
- **I2C**: SCL0/SDA0, SCL1/SDA1 - Available
- **Serial**: TX2/RX2 (pins 30/29) - Available (TX1/RX1 used for USB serial)
- **GPIO**: GPIO101, GPIO102, GPIO105-GPIO1014 - Available

## LoRa Configuration

The LoRa radio is configured via framework functions:

```cpp
Radio.Init(&radioEvents);
Radio.SetChannel(869618000);  // Frequency in Hz
Radio.SetTxConfig(MODEM_LORA, ...);
Radio.SetRxConfig(MODEM_LORA, ...);
```

The framework automatically configures:
- **MOSI0** (pin 25)
- **MISO0** (pin 26)  
- **CLK0** (pin 27)
- **NSS0** (pin 28)

## Power Management

### Battery Monitoring

The firmware monitors battery voltage using `getBatteryVoltage()`, which:
- Reads battery voltage via internal ADC
- Returns voltage in millivolts (mV)
- Detects USB power (returns ~0mV when USB powered)

### Sleep Mode

The firmware uses CubeCell's light sleep mode:
- Function: `lowPowerHandler()`
- Preserves RAM and peripheral state
- Wakes on interrupt (LoRa radio RX events)
- Typical sleep current: < 1mA

### VEXT Control

VEXT (GPIO1015, pin 19) controls external power to LED and other peripherals:
- **Controlled by**: CubeCell framework (automatic)
- **Purpose**: Powers external LED, can power external sensors
- **Note**: Framework handles VEXT automatically; no manual control needed

## Compatibility Notes

### Differences from Other CubeCell Boards

This firmware is designed for **HTCC-AB02A** and uses only framework-abstracted functions. It should be compatible with other CubeCell boards that support the same framework APIs:

- ✅ **HTCC-AB02A**: Tested and verified
- ✅ **HTCC-AB02**: Compatible (generic CubeCell board)
- ✅ **HTCC-AB01**: Compatible (older model)
- ⚠️ **HTCC-AB02S**: Compatible, but has GPS module (not used by this firmware)

### Board-Specific Features NOT Used

The following HTCC-AB02A features are **not used** by this firmware:

- GPS module (if present)
- User button (GPIO1016)
- Additional ADC channels (ADC2, ADC3)
- I2C peripherals
- Additional serial ports

## Verification Checklist

✅ All pin access goes through CubeCell framework functions  
✅ No direct GPIO pin numbers used in code  
✅ Battery monitoring uses framework `getBatteryVoltage()`  
✅ LoRa radio uses framework `Radio` API  
✅ Power management uses framework `lowPowerHandler()`  
✅ VEXT control handled automatically by framework  
✅ No unsupported pins referenced  
✅ Compatible with HTCC-AB02A pinout  

## References

- [HTCC-AB02A Official Documentation](https://docs.heltec.cn/en/node/asr650x/htcc_ab02a/)
- [CubeCell Arduino Framework](https://github.com/HelTecAutomation/CubeCell-Arduino)
- Pinout diagram: See project root

## Revision History

- **2025-12-03**: Initial documentation for HTCC-AB02A compatibility verification


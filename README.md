# MiniCore

Minimal MeshCore-compatible firmware foundation for Heltec CubeCell (HTCC-AB01) using PlatformIO. Built with TDD principles for full test coverage.

**Requirements:** [PlatformIO](https://platformio.org/) (e.g. `pip install platformio`), Python 3.x. For on-device build and flash: Heltec CubeCell board and toolchain.

## Architecture

```
MiniCore/
├── include/                    # Public headers (module layout)
│   ├── core/                   # config.h, constants.h, result.h, version.h, platform.h
│   ├── packet/                 # packet.h, packet_history.h
│   ├── routing/                # repeater.h, packet_router.h
│   ├── radio/                  # radio_controller.h
│   ├── crypto/                 # crypto_types.h, identity_manager.h, ed25519_crypto.h
│   ├── app/                    # application.h, device_factory.h
│   ├── time/                   # time_sync.h
│   ├── power/                  # power_save.h
│   ├── advert/                 # advert.h
│   ├── discovery/              # discovery.h
│   ├── queue/                  # tx_queue.h
│   ├── util/                   # circular_buffer.h, noise_floor_estimator.h
│   └── hal/                    # Hardware Abstraction Layer (i_board.h, i_crypto.h, ...)
├── lib/
│   ├── ed25519/                # Ed25519 library (MeshCore compatible; see lib/ed25519/license.txt)
│   └── sha256/
├── src/
│   ├── main.cpp                # Application entry point
│   ├── app/                    # application.cpp
│   ├── packet/                 # packet.cpp, packet_history.cpp
│   ├── routing/                # repeater.cpp, packet_router.cpp
│   ├── radio/                  # radio_controller.cpp
│   ├── crypto/                 # crypto_types.cpp, ed25519_crypto.cpp, identity_manager.cpp
│   ├── time/                   # time_sync.cpp
│   ├── power/                  # power_save.cpp
│   ├── queue/                  # tx_queue.cpp
│   ├── util/                   # noise_floor_estimator.cpp
│   └── hal/cubecell/           # CubeCell HAL implementation
├── test/
│   ├── test_native/            # Host-based tests (fast TDD cycle)
│   │   ├── mocks/              # Mock implementations for testing
│   │   ├── core/, packet/, routing/, ...  # Test files by module
│   │   └── test_main.cpp       # Test runner
│   └── test_embedded/          # On-device tests (38 tests)
├── scripts/                    # Build/test helpers (PowerShell, bash, optional strip_comments.py)
└── platformio.ini              # Build configuration
```

## Versioning

Firmware version is defined in `include/core/version.h` as `FIRMWARE_VERSION` (major.minor.patch). We follow [semantic versioning](https://semver.org/): major for incompatible changes, minor for backward-compatible features, patch for backward-compatible fixes.

## Practices

| Practice | Implementation |
|----------|----------------|
| **TDD** | Tests written first; 445+ native test cases + 38 embedded tests |
| **Clean Architecture** | HAL interfaces decouple hardware from logic |
| **SOLID** | Single-responsibility interfaces, dependency injection ready |
| **MeshCore Compatible** | Ed25519 keys, same storage format |
| **No Exceptions** | `Result<T>` for error handling, `-fno-exceptions` flag |
| **No RTTI** | Compile-time polymorphism, `-fno-rtti` flag |
| **Testable** | All interfaces have mock implementations |
| **Config vs constants** | Tunables in `include/core/config.h`; protocol/MeshCore constants in `include/core/constants.h` (do not change) |
| **Application API** | Single `context()` returns `ApplicationContext` (device, txQueue, router, identity) for handlers; no per-getter API |

## Reusable Components

### Error Handling (`core/result.h`)
```cpp
Result<int> readSensor() {
    if (!initialized) return ErrorCode::NotInitialized;
    return sensorValue;
}

auto result = readSensor();
if (result.isOk()) {
    process(result.value());
} else {
    handleError(result.error());
}
```

### Circular Buffer (`util/circular_buffer.h`)
```cpp
CircularBuffer<Packet, 16> rxQueue;
rxQueue.push(packet);           // Returns Result<void>
auto pkt = rxQueue.pop();       // Returns Result<Packet>
```

### HAL Interfaces (`hal/`)
```cpp
class MyRadioHandler : public IRadioEvents {
    void onRxDone(const RxPacket& pkt) override { /* handle */ }
    void onTxDone() override { /* next action */ }
};

void sendPacket(IRadio& radio, const uint8_t* data, size_t len) {
    radio.send(data, len);  // Works with real or mock radio
}
```

### Mocks for Testing (`test/test_native/mocks/`)
```cpp
Mocks::MockStorage storage;
storage.init(256);
storage.write(0, 0x42);
TEST_ASSERT_EQUAL(0x42, storage.read(0).value());
```

### Identity Management (`crypto/identity_manager.h`)
```cpp
Ed25519Crypto crypto;
IdentityManager manager(storage, rng, crypto, log);

auto result = manager.loadOrCreate();
if (result.isOk()) {
    LocalIdentity& identity = result.value();
}
```

## Commands

PowerShell or bash:

```bash
pio run                          # Build release firmware
pio run -e cubecell_board_debug  # Build with debug symbols
pio test -e native                # Run native host tests (~1 sec)
pio test -e cubecell_board_test   # Run on-device tests (device required)
pio run -t upload                 # Flash to device
```

Optional: `scripts/strip_comments.py` removes C/C++ comments and collapses blank lines (e.g. for size or diff checks). Run with Python 3: `python3 scripts/strip_comments.py < file.cpp`.

## Build Environments

| Environment | Purpose |
|-------------|---------|
| `cubecell_board` | Production (release build, NDEBUG) |
| `cubecell_board_debug` | Development with debug symbols (DEBUG_BUILD) |
| `cubecell_board_test` | On-device unit tests |
| `native` | Fast TDD on host PC |
| `native_coverage` | Native tests with gcov/lcov coverage |

Use `cubecell_board` for production deployment; use `cubecell_board_debug` for development and debugging.

## Adding New Features

1. Define interface in `include/hal/i_*.h`
2. Create mock in `test/test_native/mocks/`
3. Write tests (add declaration and `RUN_TEST` in `test/test_native/test_main.cpp` and implement in `test/test_native/<module>/test_*.cpp`)
4. Run tests: `pio test -e native`
5. Implement for CubeCell in `src/`
6. Verify on device: `pio test -e cubecell_board_test`

## Adding a New Device

See `src/hal/template/DEVICE_TEMPLATE.md` for step-by-step instructions and `platformio.ini` (template comment at bottom) for the new env template.

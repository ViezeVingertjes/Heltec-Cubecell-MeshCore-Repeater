# Contributing to Heltec CubeCell MeshCore Repeater

Thank you for your interest in contributing! This document provides guidelines for contributing to the project.

## Code of Conduct

- Be respectful and inclusive
- Focus on constructive feedback
- Help create a welcoming environment for all contributors

## Getting Started

1. Fork the repository
2. Clone your fork: `git clone https://github.com/yourusername/Heltec-Cubcell-MeshCore-Repeater.git`
3. Create a feature branch: `git checkout -b feature/your-feature-name`
4. Make your changes
5. Test thoroughly
6. Commit with clear messages
7. Push to your fork
8. Create a Pull Request

## Development Setup

### Prerequisites

- [PlatformIO](https://platformio.org/) installed
- Heltec CubeCell board for testing
- USB cable for programming

### Build and Test

```bash
# Debug build (with logging)
pio run -e cubecell_board_debug

# Production build
pio run -e cubecell_board

# Upload to device
pio run --target upload

# Monitor serial output
pio device monitor
```

## Coding Standards

### Style Guidelines

- Use C++11/14 features appropriately
- Follow existing code style (2-space indentation, K&R braces)
- Use meaningful variable and function names
- Add comments for complex logic
- Prefer `constexpr` for compile-time constants

### Memory Management

- **NO dynamic allocation** (no `new`, `malloc`, `std::vector`, etc.)
- Use fixed-size buffers
- Validate buffer bounds before operations
- Use stack allocation for temporary data

### Error Handling

- Use `Result<T>` type for operations that can fail
- Validate inputs before processing
- Return appropriate error codes
- Log errors with context

### Example Code Style

```cpp
namespace MeshCore {

class MyProcessor : public IPacketProcessor {
public:
  MyProcessor() : counter(0) {}

  ProcessResult processPacket(const PacketEvent &event,
                              ProcessingContext &ctx) override {
    // Validate input
    if (event.packet.payloadLength == 0) {
      return ProcessResult::CONTINUE;
    }

    // Process packet
    counter++;
    LOG_INFO_FMT("Processed packet %lu", counter);
    
    return ProcessResult::CONTINUE;
  }

  const char *getName() const override { return "MyProcessor"; }
  uint8_t getPriority() const override { return 25; }

private:
  uint32_t counter;
};

} // namespace MeshCore
```

## Testing Guidelines

### Before Submitting

1. **Build both targets successfully**
   - Debug build: `pio run -e cubecell_board_debug`
   - Production build: `pio run -e cubecell_board`

2. **Test on actual hardware**
   - Upload firmware
   - Run for at least 1 hour
   - Verify packet forwarding works
   - Check memory usage (no leaks)
   - Monitor for crashes or hangs

3. **Check for common issues**
   - No compiler warnings
   - No buffer overflows
   - Proper null checks
   - Correct bounds validation

### Test Checklist

- [ ] Compiles without warnings (both debug and production)
- [ ] Tested on actual hardware
- [ ] No memory leaks or corruption
- [ ] Proper error handling
- [ ] Logging provides useful debug info
- [ ] Power consumption acceptable
- [ ] Radio transmissions work correctly

## Pull Request Process

1. **Update documentation** if you change:
   - Configuration options
   - Public APIs
   - Behavior or features

2. **Update CHANGELOG.md** under `[Unreleased]` section

3. **Provide clear description**:
   - What does this PR do?
   - Why is this change needed?
   - How was it tested?
   - Any breaking changes?

4. **PR Title Format**: `[Component] Brief description`
   - Examples: `[Radio] Fix duty cycle tracking`, `[Docs] Update README`

5. **Keep PRs focused**: One feature/fix per PR when possible

## Areas for Contribution

### High Priority

- [ ] Implement duty cycle tracking and enforcement
- [ ] Add hardware RNG support for entropy collection
- [ ] Improve cryptographic key generation
- [ ] Add unit tests for critical functions
- [ ] OTA update capability

### Documentation

- [ ] Add Doxygen comments to public APIs
- [ ] Create usage examples
- [ ] Document network protocol details
- [ ] Add troubleshooting guide

### Testing

- [ ] Unit tests for PacketDecoder/Encoder
- [ ] Unit tests for PacketValidator
- [ ] Integration tests for packet forwarding
- [ ] Power consumption benchmarks

### Features

- [ ] Additional processor plugins
- [ ] Metrics/statistics endpoint
- [ ] Configuration via serial console
- [ ] GPS support for location tracking

## Questions or Problems?

- Open an issue for bugs or feature requests
- Tag with appropriate labels (bug, enhancement, question)
- Provide as much context as possible
- Include hardware version and firmware version

## License

By contributing, you agree that your contributions will be licensed under the MIT License.

## Recognition

Contributors will be acknowledged in the README and release notes. Thank you for helping make this project better!


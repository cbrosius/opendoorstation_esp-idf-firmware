# ESP32 SIP Door Station

A smart door intercom system built on ESP32-S3 that enables remote door access control through SIP (VoIP) calling.

## Features

- 🔊 **SIP/VoIP Integration** - Make and receive calls through standard VoIP infrastructure
- 🚪 **Door Control** - Remote door unlock via DTMF tones or web interface
- 🌐 **Wi-Fi Connectivity** - Easy network setup and management
- 🎤 **Audio Processing** - High-quality audio codec with noise reduction
- 📱 **Web Interface** - Configuration and monitoring through built-in web server
- 🔒 **Security** - Encrypted communications and secure authentication
- 🧪 **Comprehensive Testing** - Unity test framework integration

## Hardware Requirements

- **ESP32-S3** microcontroller
- **Audio Codec** (e.g., ES8388, WM8978)
- **Microphone** and **Speaker**
- **Door Lock Relay** circuit
- **Power Supply** (5V/12V depending on door lock)

## Quick Start

### Prerequisites

- [ESP-IDF v5.4.2](https://docs.espressif.com/projects/esp-idf/en/v5.4.2/esp32/get-started/index.html)
- Python 3.8+
- Git

### Setup

1. **Clone the repository:**
   ```bash
   git clone https://github.com/yourusername/opendoorstation.git
   cd opendoorstation
   ```

2. **Install ESP-IDF tools:**
   ```bash
   # Windows
   install_esp_idf.bat
   
   # Linux/Mac
   ./install_esp_idf.sh
   ```

3. **Activate ESP-IDF environment:**
   ```bash
   # Windows
   setup_env.bat
   
   # Linux/Mac
   source setup_env.sh
   ```

4. **Verify setup:**
   ```bash
   # Windows
   verify_build.bat
   
   # Linux/Mac
   ./verify_build.sh
   ```

### Build and Flash

```bash
# Configure project (optional)
idf.py menuconfig

# Build
idf.py build

# Flash to device
idf.py -p PORT flash monitor
```

Replace `PORT` with your ESP32 device port (e.g., `COM3` on Windows, `/dev/ttyUSB0` on Linux).

## Project Structure

```
├── main/                   # Main application code
│   ├── app_main.c         # Application entry point
│   └── CMakeLists.txt     # Main component build config
├── test/                   # Unit tests
│   ├── test_main.c        # Unity test framework
│   └── CMakeLists.txt     # Test component config
├── web_root/              # Static web files
│   └── index.html         # Configuration interface
├── components/            # Custom components (created as needed)
├── docs/                  # Documentation
└── scripts/               # Build and setup scripts
```

## Configuration

The device can be configured through:

1. **Web Interface** - Connect to device AP or access via network IP
2. **Serial Console** - Use `idf.py monitor` for configuration commands
3. **menuconfig** - Use `idf.py menuconfig` for build-time configuration

## Development

### Running Tests

```bash
# Build and run unit tests
idf.py -T test build flash monitor

# Run specific test
idf.py -T test -E "test_name" build flash monitor
```

### Adding Components

```bash
# Create new component
idf.py create-component components/my_component
```

### Debugging

```bash
# Enable debug logging
idf.py menuconfig
# Navigate to: Component config → Log output → Default log verbosity → Debug

# Monitor with debug output
idf.py monitor
```

## Troubleshooting

See [TROUBLESHOOTING.md](TROUBLESHOOTING.md) for common issues and solutions.

### Common Issues

- **Build errors**: Run `clean_build.bat` or `idf.py fullclean && idf.py build`
- **Flash errors**: Check device connection and port permissions
- **Wi-Fi issues**: Verify credentials and network compatibility

## Contributing

1. Fork the repository
2. Create a feature branch (`git checkout -b feature/amazing-feature`)
3. Commit your changes (`git commit -m 'Add amazing feature'`)
4. Push to the branch (`git push origin feature/amazing-feature`)
5. Open a Pull Request

### Development Guidelines

- Follow ESP-IDF coding standards
- Add unit tests for new functionality
- Update documentation for API changes
- Test on actual hardware before submitting

## License

This project is licensed under the MIT License - see the [LICENSE](LICENSE) file for details.

## Acknowledgments

- [ESP-IDF](https://github.com/espressif/esp-idf) - Espressif IoT Development Framework
- [Unity](https://github.com/ThrowTheSwitch/Unity) - Unit testing framework
- [SIP Protocol](https://tools.ietf.org/html/rfc3261) - Session Initiation Protocol specification

## Support

- 📖 [Documentation](docs/)
- 🐛 [Issue Tracker](https://github.com/yourusername/opendoorstation/issues)
- 💬 [Discussions](https://github.com/yourusername/opendoorstation/discussions)

---

**Note**: This project is under active development. Features and APIs may change.

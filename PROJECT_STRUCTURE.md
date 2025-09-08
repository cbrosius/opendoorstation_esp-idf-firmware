# ESP32 SIP Door Station Project Structure

## Directory Layout

```
sip-door-station/
├── CMakeLists.txt              # Root CMake configuration
├── sdkconfig.defaults          # ESP-IDF default configuration
├── project_description.txt     # Project description
├── main/                       # Main application component
│   ├── CMakeLists.txt         # Main component build configuration
│   └── app_main.c             # Application entry point
├── test/                       # Unit test component
│   ├── CMakeLists.txt         # Test component build configuration
│   └── test_main.c            # Unity test framework entry point
└── web_root/                   # Static web files
    └── index.html             # Configuration interface placeholder
```

## Build System

This project uses ESP-IDF's CMake-based build system with:
- Unity testing framework integration
- Component-based architecture
- Static web file embedding support
- ESP32-S3 target configuration

## Getting Started

1. Set up ESP-IDF development environment
2. Run `idf.py build` to build the project
3. Run `idf.py -p PORT flash monitor` to flash and monitor
4. For testing: `idf.py -T test build` to build test target
# Gemini Project Companion: ESP32 SIP Door Station

This document outlines the core features, development processes, and user preferences for the ESP32 SIP Door Station project. It is maintained by Gemini to ensure consistency and alignment with the project goals across development sessions.

## 1. Project Goal

The primary goal is to develop a functional prototype of a smart door station based on the ESP32-S3. The device will initiate a SIP (VoIP) call when a button is pressed and allow the user who answers the call to open a door and control a light using DTMF tones.

## 2. Core Features

- **SIP Calling:** Pressing the doorbell button initiates a SIP call to a pre-configured URI.
- **DTMF Relay Control:**
    - DTMF digit `1`: Pulses a relay to open a door.
    - DTMF digit `2`: Toggles a relay to control a light.
- **Web Configuration:** A built-in web server provides an interface to configure:
    - Wi-Fi credentials
    - SIP account details (User, Domain, Callee)
- **Virtual I/O:** The web interface includes a virtual doorbell button and live status indicators for the relays, allowing for testing the core logic without physical hardware.
- **Hardware:** ESP32-S3 DevKit (Hard-coded project target), button, and two relays.

## 3. Development Process & Preferences

This section contains specific instructions and preferences provided by the user.

### 3.1. Unit Testing (User Mandate)

**All new functions written must be accompanied by unit tests.**

- **Framework:** Unity (via ESP-IDF).
- **Location:** Tests are located in the `main/test` subdirectory.
- **Practice:** Where possible, test logic in isolation. For hardware-dependent code, mock the hardware interaction or, at a minimum, create smoke tests to ensure the function runs without errors.

### 3.2. Technology Stack

- **Firmware:** ESP-IDF
- **ESP-IDF Version:** Always use the latest stable version for generating code. (User preference)
- **SIP Library:** `esp_sip` from the ESP-ADF framework.
- **Web Server:** `esp_http_server` native to ESP-IDF.
- **JSON Parsing:** `cJSON` library.

### 3.3. Project Structure

The project follows a standard ESP-IDF component structure:

```
firmware/esp-idf/
└─── main/
     ├─── web_root/      # HTML/CSS/JS for the web UI
     ├─── test/          # Unit tests
     ├─── app_main.c     # Main application logic
     ├─── *.c, *.h       # Feature modules (SIP, I/O, etc.)
     └─── CMakeLists.txt # Component build script
```

### 3.4. Secrets Management

To handle sensitive data like Wi-Fi and SIP credentials securely, the project uses a `.env` file at the project root.

- **File:** `.env` (not committed to Git).
- **Format:** Standard `KEY="VALUE"` pairs.
- **Process:**
    1. The `.env` file in the project root is automatically loaded by the build system.
    2. The values are made available to the firmware code via VARIABLES (e.g., `WIFI_SSID`).
- **Web UI:** The web interface provides a read-only view of the configuration that was compiled into the firmware.

## 4. Future Roadmap

The following features are planned for future development, based on the initial project documentation:

- **Full Audio Integration:** Add I²S microphone and amplifier for two-way audio.
- **Enhanced Security:** Implement a DTMF PIN sequence for actions instead of a single digit.
- **Home Automation Integration:** Connect to MQTT brokers (e.g., Home Assistant) to publish status and events.
- **Hardware Evolution:**
    - Support for multiple buttons/apartments.
    - Power over Ethernet (PoE).
    - Camera integration.

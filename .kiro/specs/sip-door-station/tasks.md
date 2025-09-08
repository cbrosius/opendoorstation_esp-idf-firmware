# Implementation Plan

- [x] 1. Set up project structure and build system





  - Create ESP-IDF project directory structure with main/, test/, and web_root/ folders
  - Configure CMakeLists.txt files for main component and test framework integration
  - Set up Unity testing framework integration with ESP-IDF build system
  - Create basic app_main.c with minimal initialization
  - _Requirements: 7.1, 7.4_

- [x] 2. Implement configuration management system





  - [x] 2.1 Create configuration data structures and validation


    - Define door_station_config_t structure with all required fields
    - Implement configuration validation functions with input sanitization
    - Write unit tests for configuration validation logic
    - _Requirements: 4.3, 6.4_

  - [x] 2.2 Implement NVS-based configuration persistence


    - Create config_manager component with NVS storage operations
    - Implement config_manager_load() and config_manager_save() functions
    - Write unit tests using NVS mocks for storage operations
    - _Requirements: 4.5, 6.3_

  - [x] 2.3 Add environment variable integration for build-time configuration


    - Implement build system integration to load .env file variables
    - Create config_manager_init() to merge build-time and runtime configuration
    - Write unit tests for environment variable parsing and merging
    - _Requirements: 6.1, 6.2_

- [x] 3. Implement GPIO and I/O management





  - [x] 3.1 Create I/O manager with GPIO abstraction


    - Implement io_manager component with GPIO initialization
    - Create button monitoring with debouncing logic
    - Write unit tests using GPIO mocks for button input handling
    - _Requirements: 1.1, 5.2_

  - [x] 3.2 Implement relay control with safety features


    - Create relay control functions with pulse and toggle operations
    - Implement safety timers and state protection logic
    - Write unit tests for relay timing and protection mechanisms
    - _Requirements: 2.1, 2.3, 3.1, 3.2_

  - [x] 3.3 Add event system for I/O state changes


    - Implement event publishing for button presses and relay state changes
    - Create event handlers for I/O events in main application
    - Write unit tests for event generation and handling
    - _Requirements: 2.4, 3.3, 5.4_

- [ ] 4. Implement SIP communication system
  - [ ] 4.1 Create SIP manager with esp_sip integration
    - Initialize SIP manager component using esp_sip library
    - Implement SIP client configuration and registration
    - Write unit tests using SIP library mocks for registration flow
    - _Requirements: 1.1, 1.2_

  - [ ] 4.2 Implement call management functionality
    - Create sip_manager_start_call() and sip_manager_end_call() functions
    - Implement call state tracking and timeout handling
    - Write unit tests for call initiation and termination scenarios
    - _Requirements: 1.1, 1.2, 1.3_

  - [ ] 4.3 Add DTMF tone processing
    - Implement DTMF callback registration and tone processing
    - Create DTMF-to-relay command mapping logic
    - Write unit tests for DTMF digit recognition and command execution
    - _Requirements: 1.4, 2.1, 3.1_

  - [ ] 4.4 Integrate SIP events with I/O system
    - Connect DTMF events to relay control operations
    - Implement call state synchronization with I/O manager
    - Write integration tests for SIP-to-relay command flow
    - _Requirements: 2.1, 2.4, 3.1, 3.3_

- [ ] 5. Implement web server and user interface
  - [ ] 5.1 Create basic web server with static file serving
    - Initialize esp_http_server with static file handler
    - Create basic HTML/CSS/JS files for configuration interface
    - Write unit tests for HTTP server initialization and static serving
    - _Requirements: 4.1, 5.1_

  - [ ] 5.2 Implement configuration API endpoints
    - Create REST API endpoints for reading and updating configuration
    - Implement JSON serialization/deserialization using cJSON
    - Write unit tests for API endpoint request/response handling
    - _Requirements: 4.2, 4.3, 6.2_

  - [ ] 5.3 Add virtual doorbell and relay control API
    - Implement POST /api/doorbell endpoint for virtual button press
    - Create GET /api/relays endpoint for relay status retrieval
    - Write unit tests for virtual I/O API functionality
    - _Requirements: 5.1, 5.2, 5.3_

  - [ ] 5.4 Implement real-time status updates with WebSocket
    - Add WebSocket support for live relay status updates
    - Create client-side JavaScript for real-time UI updates
    - Write integration tests for WebSocket message delivery
    - _Requirements: 5.3, 5.4_

- [ ] 6. Implement main application controller and state management
  - [ ] 6.1 Create main application state machine
    - Implement system_state_t structure and state management
    - Create main event loop for coordinating component interactions
    - Write unit tests for state transitions and event handling
    - _Requirements: 1.1, 1.4, 4.5_

  - [ ] 6.2 Integrate all components in main application
    - Connect button events to SIP call initiation
    - Wire DTMF events to relay control operations
    - Write integration tests for complete user workflows
    - _Requirements: 1.1, 2.1, 3.1_

  - [ ] 6.3 Add error handling and logging system
    - Implement comprehensive error handling across all components
    - Create logging system for debugging and monitoring
    - Write unit tests for error scenarios and recovery mechanisms
    - _Requirements: 1.3, 2.3, 4.4_

- [ ] 7. Implement security and credential management
  - [ ] 7.1 Add secure storage for sensitive configuration
    - Implement encrypted storage for Wi-Fi and SIP credentials
    - Create credential masking for web interface display
    - Write unit tests for secure storage operations
    - _Requirements: 6.1, 6.2, 6.3_

  - [ ] 7.2 Implement factory reset functionality
    - Create factory reset function to clear all stored credentials
    - Add factory reset API endpoint and web interface button
    - Write unit tests for factory reset operations
    - _Requirements: 6.4_

- [ ] 8. Create comprehensive test suite and validation
  - [ ] 8.1 Implement hardware abstraction layer tests
    - Create mock implementations for all hardware dependencies
    - Write smoke tests for hardware-dependent functions
    - Achieve target code coverage for hardware abstraction layer
    - _Requirements: 7.2, 7.3_

  - [ ] 8.2 Add end-to-end integration tests
    - Create integration tests for complete call-to-relay workflows
    - Implement web interface functionality validation tests
    - Write configuration persistence and recovery tests
    - _Requirements: 7.1, 7.2, 7.5_

  - [ ] 8.3 Implement system performance and reliability tests
    - Create stress tests for concurrent SIP and web operations
    - Implement memory leak detection and resource usage tests
    - Write reliability tests for error recovery scenarios
    - _Requirements: 7.4, 7.5_
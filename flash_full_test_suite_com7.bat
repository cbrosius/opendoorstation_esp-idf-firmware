@echo off
REM Flash the full test suite firmware and monitor execution
REM This will run the comprehensive 60+ test suite

echo ESP32 SIP Door Station - Flash Full Test Suite (COM7)
echo ====================================================

REM Check if ESP-IDF is installed
if not exist "C:\Users\Christian\esp\v5.4.2\esp-idf\export.bat" (
    echo ERROR: ESP-IDF not found at expected location
    exit /b 1
)

echo Activating ESP-IDF environment...
call "C:\Users\Christian\esp\v5.4.2\esp-idf\export.bat" >nul 2>&1

if %errorlevel% neq 0 (
    echo ERROR: Failed to activate ESP-IDF environment
    exit /b 1
)

echo.
echo Flashing full test suite firmware to ESP32-S3 on COM7...
echo.

REM Flash the firmware
idf.py -p COM7 flash

if %errorlevel% neq 0 (
    echo FLASH FAILED
    echo Please check:
    echo 1. ESP32-S3 is connected to COM7
    echo 2. Device is not in use by another application
    echo 3. Try putting device in download mode (hold BOOT, press RESET, release BOOT)
    exit /b 1
)

echo.
echo FLASH SUCCESSFUL
echo.
echo Starting comprehensive test suite monitor...
echo.
echo Expected test output:
echo - Configuration validation and storage tests
echo - SIP manager and call handling tests  
echo - I/O manager and relay control tests
echo - Web server and API tests
echo - Hardware abstraction layer tests
echo - End-to-end integration tests
echo - Performance and reliability tests
echo.
echo Total: 60+ individual test functions
echo.
echo Press Ctrl+] to exit monitor
echo.

REM Start monitoring the test execution
idf.py -p COM7 monitor
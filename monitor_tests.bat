@echo off
REM Monitor script to watch test execution on the ESP32-S3 hardware
REM This will show the test results in real-time

echo ESP32 SIP Door Station - Test Monitor
echo =======================================

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
echo Starting test monitor...
echo.
echo The ESP32-S3 should now be running the Unity test framework.
echo You should see test results appearing below.
echo.
echo Test Categories:
echo - Configuration validation and storage tests
echo - SIP manager and call handling tests  
echo - I/O manager and relay control tests
echo - Web server and API tests
echo - Hardware abstraction layer tests
echo - End-to-end integration tests
echo - Performance and reliability tests
echo.
echo Press Ctrl+] to exit monitor
echo.

REM Start monitoring the serial output
idf.py monitor
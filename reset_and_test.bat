@echo off
REM Reset the ESP32-S3 and start fresh test execution
REM Useful if you want to restart the tests

echo ESP32 SIP Door Station - Reset and Test
echo ========================================

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
echo Resetting ESP32-S3 and starting test execution...
echo.

REM Reset the device and start monitoring
idf.py monitor --no-reset

echo.
echo If the device didn't reset automatically, press the reset button on your ESP32-S3
echo or disconnect and reconnect the USB cable.
echo.
echo Press Ctrl+] to exit monitor
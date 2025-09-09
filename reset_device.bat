@echo off
REM Reset the ESP32-S3 device and start monitoring
REM This will trigger a fresh boot and test execution

echo ESP32 SIP Door Station - Device Reset
echo =======================================

REM Check if ESP-IDF is installed
if not exist "C:\Users\Christian\esp\v5.4.2\esp-idf\export.bat" (
    echo ERROR: ESP-IDF not found at expected location
    exit /b 1
)

echo Activating ESP-IDF environment...
call "C:\Users\Christian\esp\v5.4.2\esp-idf\export.bat" >nul 2>&1

echo.
echo Resetting ESP32-S3 device on COM3...
echo.

REM Reset the device using esptool
python -m esptool --chip esp32s3 --port COM3 --baud 115200 --before default_reset --after hard_reset chip_id

if %errorlevel% neq 0 (
    echo Reset failed. Trying alternative method...
    echo.
    echo Please manually press the reset button on your ESP32-S3 device.
    echo.
    pause
)

echo.
echo Starting monitor after reset...
echo You should now see the boot sequence and test execution.
echo.
echo Press Ctrl+] to exit monitor
echo.

REM Start monitoring after reset
idf.py -p COM3 monitor
@echo off
REM Help script to prepare ESP32-S3 for flashing
REM This provides instructions for getting the device ready

echo ESP32 SIP Door Station - Prepare Device for Flash
echo ==================================================

echo.
echo To prepare your ESP32-S3 for flashing, please follow these steps:
echo.
echo 1. DISCONNECT the USB cable from the ESP32-S3
echo 2. HOLD DOWN the BOOT button (GPIO0) on the ESP32-S3
echo 3. While holding BOOT, RECONNECT the USB cable
echo 4. RELEASE the BOOT button
echo 5. The device should now be in download mode
echo.
echo Alternative method:
echo 1. Keep USB connected
echo 2. HOLD DOWN the BOOT button
echo 3. PRESS and RELEASE the RESET button (while still holding BOOT)
echo 4. RELEASE the BOOT button
echo.
echo Once the device is ready, press any key to attempt flashing...
pause

echo.
echo Attempting to flash the device...
echo.

REM Check if ESP-IDF is installed
if not exist "C:\Users\Christian\esp\v5.4.2\esp-idf\export.bat" (
    echo ERROR: ESP-IDF not found at expected location
    exit /b 1
)

echo Activating ESP-IDF environment...
call "C:\Users\Christian\esp\v5.4.2\esp-idf\export.bat" >nul 2>&1

REM Try flashing
idf.py -p COM3 flash

if %errorlevel% neq 0 (
    echo.
    echo FLASH FAILED
    echo.
    echo Troubleshooting tips:
    echo 1. Try a different USB cable
    echo 2. Try a different USB port
    echo 3. Check if the device appears in Device Manager
    echo 4. Try COM7 instead of COM3: idf.py -p COM7 flash
    echo 5. Make sure no other applications are using the serial port
    echo.
    pause
    exit /b 1
) else (
    echo.
    echo FLASH SUCCESSFUL!
    echo.
    echo Starting monitor to see test output...
    echo Press Ctrl+] to exit monitor
    echo.
    idf.py -p COM3 monitor
)
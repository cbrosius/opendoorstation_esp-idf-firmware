@echo off
REM Reset the ESP32-S3 connection and flash test firmware
REM This helps clear any port busy issues

echo ESP32 SIP Door Station - Reset and Flash (COM7)
echo =================================================

REM Check if ESP-IDF is installed
if not exist "C:\Users\Christian\esp\v5.4.2\esp-idf\export.bat" (
    echo ERROR: ESP-IDF not found at expected location
    exit /b 1
)

echo Activating ESP-IDF environment...
call "C:\Users\Christian\esp\v5.4.2\esp-idf\export.bat" >nul 2>&1

echo.
echo Step 1: Resetting ESP32-S3 connection...
echo.

REM Reset the device connection
python -m esptool --chip esp32s3 --port COM7 --before default_reset --after hard_reset chip_id

if %errorlevel% neq 0 (
    echo Reset failed. Please manually reset the device:
    echo 1. Press and release the RESET button on ESP32-S3
    echo 2. Or disconnect and reconnect USB cable
    echo.
    pause
)

echo.
echo Step 2: Flashing test firmware...
echo.

REM Flash the firmware with explicit reset options
idf.py -p COM7 flash

if %errorlevel% neq 0 (
    echo.
    echo FLASH FAILED - Trying alternative method...
    echo.
    
    REM Try with explicit esptool parameters
    python -m esptool --chip esp32s3 --port COM7 --baud 460800 --before default_reset --after hard_reset write_flash --flash_mode dio --flash_size 2MB --flash_freq 80m 0x0 build\bootloader\bootloader.bin 0x8000 build\partition_table\partition-table.bin 0x10000 build\sip_door_station.bin
    
    if %errorlevel% neq 0 (
        echo.
        echo FLASH STILL FAILED
        echo.
        echo Please try:
        echo 1. Put device in download mode: Hold BOOT, press RESET, release BOOT
        echo 2. Try a different USB cable
        echo 3. Try a different USB port
        echo.
        pause
        exit /b 1
    )
)

echo.
echo FLASH SUCCESSFUL!
echo.
echo Starting test monitor...
echo You should see test execution output below.
echo.
echo Press Ctrl+] to exit monitor
echo.

REM Start monitoring
idf.py -p COM7 monitor
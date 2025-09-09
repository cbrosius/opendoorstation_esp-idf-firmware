@echo off
REM Monitor script specifically for COM3
REM Use this if you know your ESP32-S3 is on COM3

echo ESP32 SIP Door Station - Monitor COM3
echo ======================================

REM Check if ESP-IDF is installed
if not exist "C:\Users\Christian\esp\v5.4.2\esp-idf\export.bat" (
    echo ERROR: ESP-IDF not found at expected location
    exit /b 1
)

echo Activating ESP-IDF environment...
call "C:\Users\Christian\esp\v5.4.2\esp-idf\export.bat" >nul 2>&1

echo.
echo Connecting to ESP32-S3 on COM3...
echo.
echo If connection fails:
echo 1. Check USB cable connection
echo 2. Press reset button on ESP32-S3
echo 3. Try unplugging and reconnecting USB
echo.
echo Press Ctrl+] to exit monitor
echo.

REM Connect to COM3 specifically
idf.py -p COM3 monitor
@echo off
REM Script to test connection to different COM ports
REM This will try to connect to each available port

echo ESP32 SIP Door Station - COM Port Testing
echo ===========================================

REM Check if ESP-IDF is installed
if not exist "C:\Users\Christian\esp\v5.4.2\esp-idf\export.bat" (
    echo ERROR: ESP-IDF not found at expected location
    exit /b 1
)

echo Activating ESP-IDF environment...
call "C:\Users\Christian\esp\v5.4.2\esp-idf\export.bat" >nul 2>&1

echo.
echo Testing COM3...
echo.
timeout /t 2 >nul
idf.py -p COM3 monitor --print-filter "*" --no-reset
if %errorlevel% equ 0 (
    echo COM3 connection successful!
    goto :end
)

echo.
echo COM3 failed, testing COM7...
echo.
timeout /t 2 >nul
idf.py -p COM7 monitor --print-filter "*" --no-reset
if %errorlevel% equ 0 (
    echo COM7 connection successful!
    goto :end
)

echo.
echo Both ports failed. Please check:
echo 1. ESP32-S3 is connected via USB
echo 2. Device drivers are installed
echo 3. Device is not being used by another application
echo 4. Try pressing the reset button on the ESP32-S3
echo.

:end
echo.
echo Press Ctrl+] to exit monitor when connected
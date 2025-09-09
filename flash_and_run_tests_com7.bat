@echo off
REM Flash the test firmware and start monitoring test execution on COM7
REM This will flash the test-enabled firmware and show test results

echo ESP32 SIP Door Station - Flash and Run Tests (COM7)
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
echo Flashing test firmware to ESP32-S3 on COM7...
echo.

REM Flash the firmware
idf.py -p COM7 flash

if %errorlevel% neq 0 (
    echo FLASH FAILED
    echo Please check:
    echo 1. ESP32-S3 is connected to COM7
    echo 2. Device is not in use by another application
    echo 3. Try pressing reset button during flash
    echo 4. Try putting device in download mode (hold BOOT, press RESET, release BOOT)
    exit /b 1
)

echo.
echo FLASH SUCCESSFUL
echo.
echo Starting test monitor...
echo You should see test execution output below.
echo.
echo Note: If test component is not linked, you'll see an error message.
echo This is expected with the current build configuration.
echo.
echo Press Ctrl+] to exit monitor
echo.

REM Start monitoring the test execution
idf.py -p COM7 monitor
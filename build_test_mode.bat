@echo off
REM Build the project in test mode using CONFIG_RUN_TESTS flag
REM This will compile the main application to run tests instead of the door station

echo ESP32 SIP Door Station - Build Test Mode
echo ==========================================

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
echo Configuring project for test mode...
echo.

REM Set the target
idf.py set-target esp32s3

REM Set environment variable to enable test mode
set RUN_TESTS=1

REM Build the project with test mode enabled
echo Building test firmware...
idf.py build

if %errorlevel% neq 0 (
    echo BUILD FAILED
    exit /b 1
)

echo.
echo BUILD SUCCESSFUL
echo.
echo Test firmware built successfully!
echo The firmware will run Unity tests when flashed to the device.
echo.
echo To flash and monitor:
echo   idf.py -p COM3 flash monitor
echo.
echo To build normal firmware again:
echo   .\build_with_idf.bat
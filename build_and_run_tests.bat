@echo off
REM Build the project in test mode and flash it to run tests
REM This will build and flash the Unity test framework instead of the main application

echo ESP32 SIP Door Station - Build and Run Tests
echo ==============================================

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
echo Setting up test configuration...
echo.

REM Set the target
idf.py set-target esp32s3

REM Configure for testing - we need to modify the main component to run tests
echo Configuring project for test execution...

REM Build the project
echo Building test firmware...
idf.py build

if %errorlevel% neq 0 (
    echo BUILD FAILED
    exit /b 1
)

echo.
echo BUILD SUCCESSFUL
echo.
echo Flashing test firmware to ESP32-S3...
idf.py -p COM3 flash

if %errorlevel% neq 0 (
    echo FLASH FAILED
    echo Please check:
    echo 1. ESP32-S3 is connected to COM3
    echo 2. Device is not in use by another application
    echo 3. Try pressing reset button during flash
    exit /b 1
)

echo.
echo FLASH SUCCESSFUL
echo.
echo Starting test monitor...
echo You should see Unity test framework output below.
echo.
echo Press Ctrl+] to exit monitor
echo.

REM Start monitoring the test execution
idf.py -p COM3 monitor
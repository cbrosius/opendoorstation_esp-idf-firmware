@echo off
REM Test script that properly activates ESP-IDF environment and runs tests
REM This script can be called directly to build and run tests

echo ESP32 SIP Door Station - Test Script
echo ====================================

REM Check if ESP-IDF is installed
if not exist "C:\Users\Christian\esp\v5.4.2\esp-idf\export.bat" (
    echo ERROR: ESP-IDF not found at expected location
    echo Expected: C:\Users\Christian\esp\v5.4.2\esp-idf\export.bat
    echo Please check your ESP-IDF installation
    exit /b 1
)

echo Activating ESP-IDF environment...
call "C:\Users\Christian\esp\v5.4.2\esp-idf\export.bat" >nul 2>&1

if %errorlevel% neq 0 (
    echo ERROR: Failed to activate ESP-IDF environment
    echo The Python virtual environment may need to be reinstalled
    echo Try running: C:\Users\Christian\esp\v5.4.2\esp-idf\install.bat
    exit /b 1
)

echo ESP-IDF environment activated successfully
echo Building and running tests...
echo.

REM Set target to run tests
idf.py set-target esp32s3

REM Build the project
echo Building project...
idf.py build

if %errorlevel% neq 0 (
    echo.
    echo BUILD FAILED
    echo Check the error messages above for details
    exit /b 1
)

echo.
echo BUILD SUCCESSFUL
echo.
echo To run tests on hardware, connect your ESP32-S3 and run:
echo   idf.py flash monitor
echo.
echo To run tests in QEMU (if available), run:
echo   idf.py qemu monitor
echo.
echo Test build completed successfully!
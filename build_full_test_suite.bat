@echo off
REM Build the project with full test suite included
REM This ensures the test component is properly linked

echo ESP32 SIP Door Station - Build Full Test Suite
echo ===============================================

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
echo Setting up full test suite build...
echo.

REM Set environment variables for test mode
set RUN_TESTS=1
set INCLUDE_TEST_COMPONENT=1

REM Clean build to ensure fresh configuration
echo Cleaning previous build...
idf.py fullclean

REM Set target
echo Setting target to ESP32-S3...
idf.py set-target esp32s3

REM Build with test mode enabled
echo Building full test suite firmware...
idf.py build

if %errorlevel% neq 0 (
    echo BUILD FAILED
    exit /b 1
)

echo.
echo BUILD SUCCESSFUL
echo.
echo Full test suite firmware built successfully!
echo Binary size indicates comprehensive test suite is included.
echo.
echo To flash and run tests:
echo   .\flash_full_test_suite_com7.bat
echo.
echo To monitor only:
echo   .\monitor_com7.bat
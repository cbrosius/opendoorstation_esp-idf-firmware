@echo off
REM Build script that properly activates ESP-IDF environment
REM This script can be called directly to build the project

echo ESP32 SIP Door Station - Build Script
echo =====================================

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
echo Building project...
echo.

REM Run the build command
idf.py build

if %errorlevel% neq 0 (
    echo.
    echo BUILD FAILED
    echo Check the error messages above for details
    exit /b 1
) else (
    echo.
    echo BUILD SUCCESSFUL
    echo Project built successfully!
)
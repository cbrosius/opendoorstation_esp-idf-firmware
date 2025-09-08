@echo off
REM Switch to minimal configuration to resolve build issues

echo ESP32 SIP Door Station - Switching to Minimal Configuration
echo ============================================================

REM Check ESP-IDF environment
if "%IDF_PATH%"=="" (
    echo ERROR: ESP-IDF environment not set
    echo Please run: setup_env.bat or activate ESP-IDF environment
    exit /b 1
)

echo Backing up current configuration...
if exist "sdkconfig.defaults" (
    copy sdkconfig.defaults sdkconfig.defaults.backup
    echo ✓ Backed up sdkconfig.defaults to sdkconfig.defaults.backup
)

echo Switching to minimal configuration...
copy sdkconfig.minimal sdkconfig.defaults
echo ✓ Applied minimal configuration

echo Cleaning build artifacts...
if exist "build\" rmdir /s /q build
if exist "sdkconfig" del sdkconfig
if exist "sdkconfig.old" del sdkconfig.old

echo Reconfiguring with minimal settings...
idf.py reconfigure

if %errorlevel% neq 0 (
    echo ERROR: Reconfiguration failed
    exit /b 1
)

echo ✓ Reconfigured with minimal settings

echo.
echo Testing build with minimal configuration...
idf.py build

if %errorlevel% equ 0 (
    echo.
    echo ✓ Build successful with minimal configuration!
    echo You can now add features incrementally using: idf.py menuconfig
) else (
    echo.
    echo ✗ Build still failing
    echo This may indicate a deeper ESP-IDF or toolchain issue
)

echo.
pause
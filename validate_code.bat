@echo off
REM Simple code validation script
REM This script performs basic syntax checks without requiring ESP-IDF environment

echo ESP32 SIP Door Station - Code Validation
echo ==========================================

echo Checking file structure...

REM Check if main files exist
if not exist "main\config_manager.h" (
    echo ERROR: main\config_manager.h not found
    exit /b 1
)
if not exist "main\config_manager.c" (
    echo ERROR: main\config_manager.c not found
    exit /b 1
)
if not exist "main\app_main.c" (
    echo ERROR: main\app_main.c not found
    exit /b 1
)

echo ✓ Main source files found

REM Check if test files exist
if not exist "test\test_config_manager.c" (
    echo ERROR: test\test_config_manager.c not found
    exit /b 1
)
if not exist "test\test_config_storage.c" (
    echo ERROR: test\test_config_storage.c not found
    exit /b 1
)
if not exist "test\test_config_env.c" (
    echo ERROR: test\test_config_env.c not found
    exit /b 1
)

echo ✓ Test files found

REM Check if CMakeLists.txt files exist
if not exist "CMakeLists.txt" (
    echo ERROR: Root CMakeLists.txt not found
    exit /b 1
)
if not exist "main\CMakeLists.txt" (
    echo ERROR: main\CMakeLists.txt not found
    exit /b 1
)
if not exist "test\CMakeLists.txt" (
    echo ERROR: test\CMakeLists.txt not found
    exit /b 1
)

echo ✓ CMakeLists.txt files found

REM Check for basic syntax issues (very basic check)
findstr /C:"#include" main\*.c >nul
if %errorlevel% neq 0 (
    echo WARNING: No include statements found in main source files
)

findstr /C:"esp_err_t" main\*.c >nul
if %errorlevel% equ 0 (
    echo ✓ ESP-IDF types found in source
)

findstr /C:"TEST_ASSERT" test\*.c >nul
if %errorlevel% equ 0 (
    echo ✓ Unity test assertions found in tests
)

echo.
echo Code structure validation completed successfully!
echo.
echo To build the project:
echo   1. Open ESP-IDF Command Prompt
echo   2. Navigate to project directory
echo   3. Run: idf.py build
echo.
echo To run tests:
echo   1. Run: idf.py -C test build
echo   2. Run: idf.py -C test flash monitor
echo.

pause
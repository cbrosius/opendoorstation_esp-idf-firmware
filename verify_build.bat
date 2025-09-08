@echo off
REM ESP-IDF Build System Verification Script for Windows
REM Run this script after setting up ESP-IDF environment

echo ESP32 SIP Door Station - Build System Verification
echo ==================================================

REM Check ESP-IDF environment
if "%IDF_PATH%"=="" (
    echo ERROR: ESP-IDF environment not set. Please run install.bat and export.bat
    exit /b 1
)

echo ESP-IDF Path: %IDF_PATH%

REM Check idf.py availability
where idf.py >nul 2>nul
if %errorlevel% neq 0 (
    echo ERROR: idf.py not found in PATH
    echo Please activate ESP-IDF environment first:
    echo   1. Open ESP-IDF Command Prompt, or
    echo   2. Run: %IDF_PATH%\export.bat
    exit /b 1
)

echo Testing idf.py availability...
idf.py --version >nul 2>nul
if %errorlevel% neq 0 (
    echo ERROR: ESP-IDF environment not properly activated
    echo Please run this script from ESP-IDF Command Prompt or after running:
    echo   %IDF_PATH%\export.bat
    echo.
    echo Current ESP-IDF Path: %IDF_PATH%
    echo.
    echo To fix this issue:
    echo   1. Open "ESP-IDF 5.4.2 CMD" from Start Menu, or
    echo   2. In regular command prompt, run: %IDF_PATH%\export.bat
    echo   3. Then run this verification script again
    exit /b 1
)

echo ✓ ESP-IDF environment properly activated

REM Verify project structure
echo.
echo Verifying project structure...

if exist "main\" (
    echo ✓ main\ directory exists
) else (
    echo ✗ main\ directory missing
    exit /b 1
)

if exist "test\" (
    echo ✓ test\ directory exists
) else (
    echo ✗ test\ directory missing
    exit /b 1
)

if exist "web_root\" (
    echo ✓ web_root\ directory exists
) else (
    echo ✗ web_root\ directory missing
    exit /b 1
)

if exist "CMakeLists.txt" (
    echo ✓ CMakeLists.txt exists
) else (
    echo ✗ CMakeLists.txt missing
    exit /b 1
)

if exist "main\CMakeLists.txt" (
    echo ✓ main\CMakeLists.txt exists
) else (
    echo ✗ main\CMakeLists.txt missing
    exit /b 1
)

if exist "main\app_main.c" (
    echo ✓ main\app_main.c exists
) else (
    echo ✗ main\app_main.c missing
    exit /b 1
)

REM Clean any previous build artifacts
echo.
echo Cleaning previous build artifacts...
idf.py fullclean >nul 2>nul

REM Test build configuration
echo Testing build configuration...
idf.py reconfigure

if %errorlevel% equ 0 (
    echo ✓ Build configuration successful
) else (
    echo ✗ Build configuration failed
    echo.
    echo Common solutions:
    echo 1. Check sdkconfig.defaults for invalid settings
    echo 2. Run: idf.py menuconfig to review configuration
    echo 3. Delete build/ directory and try again
    exit /b 1
)

REM Test main application build
echo.
echo Testing main application build...
idf.py build

if %errorlevel% equ 0 (
    echo ✓ Main application build successful
) else (
    echo ✗ Main application build failed
    echo.
    echo Build failed. Common solutions:
    echo 1. Check build logs above for specific errors
    echo 2. Run: idf.py fullclean then idf.py build
    echo 3. Check sdkconfig.defaults for configuration issues
    echo 4. Run: idf.py menuconfig to adjust settings
    echo.
    echo Build logs are saved in build/log/ directory
    exit /b 1
)

REM Test Unity test framework build
echo.
echo Testing Unity test framework build...
idf.py -T test build

if %errorlevel% equ 0 (
    echo ✓ Unity test framework build successful
) else (
    echo ✗ Unity test framework build failed
    echo Check test component configuration in test/CMakeLists.txt
    exit /b 1
)

echo.
echo All verification tests passed! ✓
echo Project structure and build system are properly configured.
echo.
echo Next steps:
echo   idf.py menuconfig     - Configure project settings
echo   idf.py -p COM_PORT flash monitor - Flash and monitor device
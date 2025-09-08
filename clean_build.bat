@echo off
REM Complete build cleanup and reconfiguration script

echo ESP32 SIP Door Station - Complete Build Cleanup
echo =================================================

REM Check ESP-IDF environment
if "%IDF_PATH%"=="" (
    echo ERROR: ESP-IDF environment not set
    echo Please run: setup_env.bat or activate ESP-IDF environment
    exit /b 1
)

echo Performing complete build cleanup...

REM Remove build directory completely
if exist "build\" (
    echo Removing build directory...
    rmdir /s /q build
    echo ✓ Build directory removed
)

REM Remove sdkconfig file (forces regeneration from defaults)
if exist "sdkconfig" (
    echo Removing cached sdkconfig...
    del sdkconfig
    echo ✓ Cached sdkconfig removed
)

REM Remove sdkconfig.old backup
if exist "sdkconfig.old" (
    echo Removing sdkconfig backup...
    del sdkconfig.old
    echo ✓ Sdkconfig backup removed
)

echo.
echo Reconfiguring project from sdkconfig.defaults...
idf.py reconfigure

if %errorlevel% neq 0 (
    echo ERROR: Reconfiguration failed
    echo Check sdkconfig.defaults for syntax errors
    exit /b 1
)

echo ✓ Project reconfigured successfully

echo.
echo Attempting clean build...
idf.py build

if %errorlevel% equ 0 (
    echo.
    echo ✓ Build completed successfully!
    echo Project is ready for development
) else (
    echo.
    echo ✗ Build failed
    echo Check the error messages above
    echo Try running: idf.py menuconfig to review configuration
)

echo.
pause
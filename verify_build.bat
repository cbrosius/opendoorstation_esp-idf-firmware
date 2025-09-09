@echo off
REM Quick verification script to check if the project builds successfully
REM This is a lightweight version that just verifies the build works

echo ESP32 SIP Door Station - Build Verification
echo ============================================

REM Check if ESP-IDF is installed
if not exist "C:\Users\Christian\esp\v5.4.2\esp-idf\export.bat" (
    echo ERROR: ESP-IDF not found at expected location
    echo Please run setup_env.bat first
    exit /b 1
)

echo Activating ESP-IDF environment...
call "C:\Users\Christian\esp\v5.4.2\esp-idf\export.bat" >nul 2>&1

if %errorlevel% neq 0 (
    echo ERROR: Failed to activate ESP-IDF environment
    exit /b 1
)

echo Verifying project configuration...
idf.py --version >nul 2>&1
if %errorlevel% neq 0 (
    echo ERROR: idf.py not available
    exit /b 1
)

echo Running build verification...
idf.py build
if %errorlevel% neq 0 (
    echo BUILD FAILED
    exit /b 1
) else (
    echo BUILD SUCCESSFUL
)

echo.
echo ✓ ESP-IDF environment: OK
echo ✓ Project configuration: OK  
echo ✓ Build system: OK
echo.
echo Project is ready for development!
echo.
echo Available commands:
echo   build_with_idf.bat  - Build the project
echo   test_with_idf.bat   - Build and prepare tests
echo   idf.py flash        - Flash to device (after build)
echo   idf.py monitor      - Monitor device output
@echo off
REM ESP-IDF Environment Setup Helper
REM This script activates the ESP-IDF environment and opens a command prompt

echo ESP32 SIP Door Station - Environment Setup
echo ==========================================

REM Check if ESP-IDF is installed
if not exist "C:\Users\Christian\esp\v5.4.2\esp-idf\export.bat" (
    echo ERROR: ESP-IDF not found at expected location
    echo Expected: C:\Users\Christian\esp\v5.4.2\esp-idf\export.bat
    echo Please check your ESP-IDF installation
    pause
    exit /b 1
)

echo Checking ESP-IDF Python environment...
if not exist "C:\Users\Christian\.espressif\python_env\idf5.4_py3.13_env\Scripts\python.exe" (
    echo WARNING: ESP-IDF Python virtual environment not found
    echo This needs to be installed first.
    echo.
    echo Running ESP-IDF installation script...
    call "C:\Users\Christian\esp\v5.4.2\esp-idf\install.bat"
    
    if %errorlevel% neq 0 (
        echo ERROR: ESP-IDF installation failed
        echo Please check the installation logs above
        pause
        exit /b 1
    )
    
    echo ✓ ESP-IDF installation completed
)

echo Activating ESP-IDF environment...
call "C:\Users\Christian\esp\v5.4.2\esp-idf\export.bat"

if %errorlevel% neq 0 (
    echo ERROR: Failed to activate ESP-IDF environment
    echo The Python virtual environment may need to be reinstalled
    echo Try running: C:\Users\Christian\esp\v5.4.2\esp-idf\install.bat
    pause
    exit /b 1
)

echo ✓ ESP-IDF environment activated successfully
echo.
echo You can now run:
echo   verify_build.bat    - Verify project setup
echo   idf.py build        - Build the project
echo   idf.py menuconfig   - Configure project settings
echo.
echo Project directory: %CD%
echo ESP-IDF Path: %IDF_PATH%
echo.

REM Keep the command prompt open
cmd /k
@echo off
REM ESP-IDF Installation Helper
REM This script installs ESP-IDF tools and Python environment

echo ESP32 SIP Door Station - ESP-IDF Installation
echo ===============================================

REM Check if ESP-IDF directory exists
if not exist "C:\Users\Christian\esp\v5.4.2\esp-idf\install.bat" (
    echo ERROR: ESP-IDF installation directory not found
    echo Expected: C:\Users\Christian\esp\v5.4.2\esp-idf\
    echo.
    echo Please download and install ESP-IDF v5.4.2 first:
    echo https://docs.espressif.com/projects/esp-idf/en/v5.4.2/esp32/get-started/windows-setup.html
    pause
    exit /b 1
)

echo Found ESP-IDF installation at: C:\Users\Christian\esp\v5.4.2\esp-idf\
echo.

REM Check if Python virtual environment already exists
if exist "C:\Users\Christian\.espressif\python_env\idf5.4_py3.13_env\Scripts\python.exe" (
    echo ✓ ESP-IDF Python virtual environment already exists
    echo.
    echo Testing environment activation...
    call "C:\Users\Christian\esp\v5.4.2\esp-idf\export.bat"
    
    if %errorlevel% equ 0 (
        echo ✓ ESP-IDF environment is working correctly
        echo.
        echo You can now run:
        echo   verify_build.bat    - Verify project setup
        echo   idf.py build        - Build the project
        pause
        exit /b 0
    ) else (
        echo ⚠ Environment activation failed, reinstalling...
    )
)

echo Installing ESP-IDF tools and Python environment...
echo This may take several minutes...
echo.

REM Run the ESP-IDF installation script
call "C:\Users\Christian\esp\v5.4.2\esp-idf\install.bat"

if %errorlevel% neq 0 (
    echo.
    echo ERROR: ESP-IDF installation failed
    echo Please check the error messages above and try again
    echo.
    echo Common solutions:
    echo 1. Run as Administrator
    echo 2. Check internet connection
    echo 3. Disable antivirus temporarily
    echo 4. Clear %USERPROFILE%\.espressif and try again
    pause
    exit /b 1
)

echo.
echo ✓ ESP-IDF installation completed successfully!
echo.

REM Test the installation
echo Testing ESP-IDF environment...
call "C:\Users\Christian\esp\v5.4.2\esp-idf\export.bat"

if %errorlevel% equ 0 (
    echo ✓ ESP-IDF environment activated successfully
    echo.
    echo Installation complete! You can now:
    echo   1. Run verify_build.bat to test the project setup
    echo   2. Use idf.py commands to build and flash
    echo.
    echo ESP-IDF Path: %IDF_PATH%
) else (
    echo ⚠ Environment activation test failed
    echo Installation may be incomplete
)

echo.
pause
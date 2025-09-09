@echo off
REM Diagnostic script to check COM port status and availability
REM This helps identify which ports are available and what might be using them

echo ESP32 SIP Door Station - COM Port Diagnostics
echo ===============================================

REM Check if ESP-IDF is installed
if not exist "C:\Users\Christian\esp\v5.4.2\esp-idf\export.bat" (
    echo ERROR: ESP-IDF not found at expected location
    exit /b 1
)

echo Activating ESP-IDF environment...
call "C:\Users\Christian\esp\v5.4.2\esp-idf\export.bat" >nul 2>&1

echo.
echo 1. Checking available serial ports...
echo.
python -m serial.tools.list_ports -v

echo.
echo 2. Checking Windows Device Manager COM ports...
echo.
wmic path Win32_SerialPort get DeviceID,Description,Name

echo.
echo 3. Testing ESP32-S3 connection on different ports...
echo.

echo Testing COM3...
python -m esptool --chip esp32s3 --port COM3 chip_id 2>nul
if %errorlevel% equ 0 (
    echo ✓ COM3: ESP32-S3 detected and accessible
) else (
    echo ✗ COM3: Not accessible or no ESP32-S3 found
)

echo.
echo Testing COM7...
python -m esptool --chip esp32s3 --port COM7 chip_id 2>nul
if %errorlevel% equ 0 (
    echo ✓ COM7: ESP32-S3 detected and accessible
) else (
    echo ✗ COM7: Not accessible or no ESP32-S3 found
)

echo.
echo 4. Checking for processes that might be using serial ports...
echo.
tasklist /fi "imagename eq putty.exe" 2>nul | find "putty.exe" >nul && echo Found: PuTTY (might be using COM port)
tasklist /fi "imagename eq arduino.exe" 2>nul | find "arduino.exe" >nul && echo Found: Arduino IDE (might be using COM port)
tasklist /fi "imagename eq code.exe" 2>nul | find "code.exe" >nul && echo Found: VS Code (might have serial monitor open)
tasklist /fi "imagename eq idf_monitor.exe" 2>nul | find "idf_monitor.exe" >nul && echo Found: ESP-IDF Monitor (using COM port)

echo.
echo 5. Recommendations:
echo.
echo If COM7 is busy:
echo   - Close any serial monitor applications (PuTTY, Arduino IDE, etc.)
echo   - Close any ESP-IDF monitor windows
echo   - Disconnect and reconnect the USB cable
echo   - Try a different USB port
echo.
echo If no ESP32-S3 is detected:
echo   - Check USB cable connection
echo   - Try a different USB cable
echo   - Check if device appears in Device Manager
echo   - Try putting device in download mode (hold BOOT, press RESET, release BOOT)
echo.

pause
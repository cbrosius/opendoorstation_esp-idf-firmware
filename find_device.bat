@echo off
REM Script to find and identify the ESP32-S3 device port
REM This will help identify which COM port your device is connected to

echo ESP32 SIP Door Station - Device Detection
echo ===========================================

REM Check if ESP-IDF is installed
if not exist "C:\Users\Christian\esp\v5.4.2\esp-idf\export.bat" (
    echo ERROR: ESP-IDF not found at expected location
    exit /b 1
)

echo Activating ESP-IDF environment...
call "C:\Users\Christian\esp\v5.4.2\esp-idf\export.bat" >nul 2>&1

echo.
echo Scanning for ESP32 devices...
echo.

REM List all available serial ports
python -m serial.tools.list_ports

echo.
echo If you see your ESP32-S3 device listed above, note the COM port number.
echo.
echo Common ESP32-S3 device identifiers:
echo - USB-SERIAL CH340 (CH340 chip)
echo - Silicon Labs CP210x (CP2102 chip) 
echo - FTDI USB Serial Device (FTDI chip)
echo - USB Serial Device (generic)
echo.
echo To manually specify a port, use:
echo   idf.py -p COMX monitor
echo.
echo Where COMX is your device's COM port (e.g., COM3, COM4, etc.)
echo.

pause
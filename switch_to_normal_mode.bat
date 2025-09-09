@echo off
REM Switch the project back to normal mode (door station application)
REM This restores the main application as the primary entry point

echo ESP32 SIP Door Station - Switch to Normal Mode
echo ================================================

echo Switching to normal mode...

REM Restore the original main file
if exist "main\app_main_disabled.c" (
    echo Restoring original app_main.c...
    ren "main\app_main_disabled.c" "app_main.c"
    echo Normal mode activated successfully!
    echo.
    echo The project will now run the SIP door station application.
    echo.
    echo To build and run the application:
    echo   .\build_with_idf.bat
    echo   idf.py -p COM3 flash monitor
) else (
    echo No disabled app_main.c found. Already in normal mode?
)

echo.
echo Normal mode is now active.
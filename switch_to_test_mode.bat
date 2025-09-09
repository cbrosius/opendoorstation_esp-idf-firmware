@echo off
REM Switch the project to test mode by renaming main files
REM This allows the test framework to be the primary entry point

echo ESP32 SIP Door Station - Switch to Test Mode
echo ==============================================

echo Switching to test mode...

REM Backup the original main file
if exist "main\app_main.c" (
    if not exist "main\app_main_backup.c" (
        echo Backing up original app_main.c...
        copy "main\app_main.c" "main\app_main_backup.c" >nul
    )
    echo Renaming app_main.c to disable it...
    ren "main\app_main.c" "app_main_disabled.c"
)

REM Check if test_main.c exists and has app_main function
if exist "test\test_main.c" (
    echo Test mode activated successfully!
    echo.
    echo The project will now run Unity tests instead of the main application.
    echo.
    echo To build and run tests:
    echo   .\build_and_run_tests.bat
    echo.
    echo To switch back to normal mode:
    echo   .\switch_to_normal_mode.bat
) else (
    echo ERROR: test_main.c not found!
    exit /b 1
)

echo.
echo Test mode is now active.
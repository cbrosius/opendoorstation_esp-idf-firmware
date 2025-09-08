@echo off
REM Build script that demonstrates .env file usage
REM This script shows how to build the project with environment variables

echo ESP32 SIP Door Station - Build with Environment Variables
echo =========================================================

REM Check if .env file exists
if not exist ".env" (
    echo WARNING: .env file not found
    echo Creating .env from .env.example...
    if exist ".env.example" (
        copy ".env.example" ".env"
        echo ✓ Created .env file from template
        echo Please edit .env file with your configuration before building
        pause
        exit /b 0
    ) else (
        echo ERROR: .env.example not found
        echo Please create .env file manually
        pause
        exit /b 1
    )
)

echo Found .env file, loading configuration...
echo.

REM Display current .env configuration (without passwords)
echo Current configuration:
for /f "tokens=1,2 delims==" %%a in (.env) do (
    if not "%%a"=="" (
        if not "%%a:~0,1%"=="#" (
            echo   %%a = [configured]
        )
    )
)

echo.
echo Building project with environment configuration...
echo.

REM Check if ESP-IDF environment is activated
where idf.py >nul 2>&1
if %errorlevel% neq 0 (
    echo ERROR: ESP-IDF environment not activated
    echo Please run this script from ESP-IDF Command Prompt or after running:
    echo   C:\Users\Christian\esp\v5.4.2\esp-idf\export.bat
    pause
    exit /b 1
)

REM Build the project
idf.py build

if %errorlevel% equ 0 (
    echo.
    echo ✓ Build completed successfully
    echo The firmware includes configuration from .env file
) else (
    echo.
    echo ✗ Build failed
    echo Please check the error messages above
)

pause
@echo off
REM Git repository setup script for ESP32 SIP Door Station

echo ESP32 SIP Door Station - Git Setup
echo ====================================

REM Check if git is available
where git >nul 2>nul
if %errorlevel% neq 0 (
    echo ERROR: Git is not installed or not in PATH
    echo Please install Git from: https://git-scm.com/
    pause
    exit /b 1
)

echo Git version:
git --version

REM Check if already in a git repository
git status >nul 2>nul
if %errorlevel% equ 0 (
    echo ✓ Already in a Git repository
    echo Current status:
    git status --short
) else (
    echo Initializing Git repository...
    git init
    echo ✓ Git repository initialized
)

REM Add all files to staging
echo.
echo Adding files to Git...
git add .

REM Show what will be committed
echo.
echo Files to be committed:
git status --short

echo.
echo Ready to commit! You can now run:
echo   git commit -m "Initial ESP-IDF project setup"
echo   git remote add origin https://github.com/yourusername/opendoorstation.git
echo   git branch -M main
echo   git push -u origin main
echo.

REM Ask if user wants to commit now
set /p commit="Do you want to commit these changes now? (y/n): "
if /i "%commit%"=="y" (
    git commit -m "Initial ESP-IDF project setup with build system and configuration"
    echo ✓ Initial commit created
    echo.
    echo Next steps:
    echo 1. Create a repository on GitHub
    echo 2. Add remote: git remote add origin [your-repo-url]
    echo 3. Push: git push -u origin main
) else (
    echo Skipping commit. Files are staged and ready when you are.
)

echo.
pause
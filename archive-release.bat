@echo off
setlocal EnableDelayedExpansion

REM Archive the dist folder into a versioned zip file
REM Output: dist\plainmd-<version>-release.zip

set "SCRIPT_DIR=%~dp0"
cd /d "%SCRIPT_DIR%"

REM Check if dist folder exists
if not exist "dist\" (
    echo ============================================
    echo ERROR: dist\ folder not found!
    echo ============================================
    echo.
    echo Please build distribution packages first:
    echo.
    echo   Windows:  build-installer.bat
    echo   Linux:    build-deb.sh
    echo   Linux:    build-appimage.sh
    echo ============================================
    pause
    exit /b 1
)

REM Check if dist folder has any files
set "HAS_FILES=0"
for /f "delims=" %%a in ('dir /b /a-d "dist\" 2^>nul') do (
    set "HAS_FILES=1"
)

if "%HAS_FILES%"=="0" (
    echo ============================================
    echo ERROR: dist\ folder is empty!
    echo ============================================
    echo.
    echo Please build distribution packages first.
    echo ============================================
    pause
    exit /b 1
)

REM Extract version from src\main.cpp
REM Look for: app.setApplicationVersion("1.2.0");
set "VERSION="
for /f "delims=" %%a in ('findstr /r /c:"setApplicationVersion.*[0-9][0-9]*\.[0-9][0-9]*\.[0-9][0-9]*" src\main.cpp 2^>nul') do (
    set "LINE=%%a"
    REM Extract version number between quotes
    for /f "tokens=2 delims=()" %%b in ("%%a") do (
        set "VER_TEMP=%%b"
        REM Remove quotes
        set "VERSION=!VER_TEMP:"=!"
    )
)

if "!VERSION!"=="" (
    echo Warning: Could not extract version from src\main.cpp
    echo Using 'unknown' as version.
    set "VERSION=unknown"
)

set "OUTPUT_FILE=plainmd-!VERSION!-release.zip"

echo ============================================
echo Creating release archive
echo ============================================
echo Version: !VERSION!
echo Source:  dist\
echo Output:  dist\!OUTPUT_FILE!
echo ============================================
echo.

REM Change to dist folder
cd dist

REM Remove old archive if exists
if exist "!OUTPUT_FILE!" del /f "!OUTPUT_FILE!"

REM Check if tar command is available (Windows 10+)
tar --version >nul 2>&1
if errorlevel 1 (
    echo ERROR: 'tar' command not found.
    echo.
    echo This script requires Windows 10 or later with built-in tar support.
    echo.
    echo Alternative: Manually zip the dist folder and rename to:
    echo   !OUTPUT_FILE!
    echo ============================================
    pause
    exit /b 1
)

REM Create zip archive using tar
REM -cf = create file, --exclude to exclude the zip itself
tar -acf "!OUTPUT_FILE!" --exclude="!OUTPUT_FILE!" *

if errorlevel 1 (
    echo ERROR: Failed to create archive.
    pause
    exit /b 1
)

echo.
echo ============================================
echo Archive created successfully!
echo ============================================
dir /b "!OUTPUT_FILE!"
echo.
echo Archive contents:
tar -tf "!OUTPUT_FILE!" | findstr /v /c:"^!OUTPUT_FILE!$"
echo ============================================

pause

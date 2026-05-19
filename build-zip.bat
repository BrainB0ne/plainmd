@echo off
setlocal EnableDelayedExpansion

REM PlainMD Portable ZIP Builder for Windows
REM Creates a ZIP distribution with all required files
REM Output: dist\plainmd-<version>-x64-portable.zip

set "SCRIPT_DIR=%~dp0"
cd /d "%SCRIPT_DIR%"

REM Check if release folder exists
if not exist "release\plainmd.exe" (
    echo ============================================
    echo ERROR: release\plainmd.exe not found!
    echo ============================================
    echo.
    echo Please build the project first:
    echo.
    echo   build.bat
    echo ============================================
    pause
    exit /b 1
)

REM Extract version from src\main.cpp
REM Look for: app.setApplicationVersion("1.3.2");
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

set "OUTPUT_FILE=plainmd-!VERSION!-x64-portable.zip"
set "TEMP_DIR=temp_zip_build"

REM Create temp directory for staging files
echo ============================================
echo Preparing portable ZIP package
echo ============================================
echo Version: !VERSION!
echo Output:  dist\!OUTPUT_FILE!
echo ============================================
echo.

if exist "%TEMP_DIR%" rmdir /s /q "%TEMP_DIR%"
mkdir "%TEMP_DIR%"

REM Copy main executable
echo Copying plainmd.exe...
copy "release\plainmd.exe" "%TEMP_DIR%\" >nul

REM Copy Qt6 Core DLLs
echo Copying Qt6 DLLs...
copy "release\Qt6Core.dll" "%TEMP_DIR%\" >nul
copy "release\Qt6Gui.dll" "%TEMP_DIR%\" >nul
copy "release\Qt6Network.dll" "%TEMP_DIR%\" >nul
copy "release\Qt6PrintSupport.dll" "%TEMP_DIR%\" >nul
copy "release\Qt6Svg.dll" "%TEMP_DIR%\" >nul
copy "release\Qt6Widgets.dll" "%TEMP_DIR%\" >nul

REM Copy additional runtime libraries
echo Copying runtime libraries...
if exist "release\D3Dcompiler_47.dll" copy "release\D3Dcompiler_47.dll" "%TEMP_DIR%\" >nul
if exist "release\opengl32sw.dll" copy "release\opengl32sw.dll" "%TEMP_DIR%\" >nul

REM Copy Qt plugins - Platforms
echo Copying Qt plugins...
if exist "release\platforms" (
    mkdir "%TEMP_DIR%\platforms"
    xcopy "release\platforms\*" "%TEMP_DIR%\platforms\" /s /e /y >nul 2>&1
)

REM Copy Qt plugins - Image formats
if exist "release\imageformats" (
    mkdir "%TEMP_DIR%\imageformats"
    xcopy "release\imageformats\*" "%TEMP_DIR%\imageformats\" /s /e /y >nul 2>&1
)

REM Copy Qt plugins - TLS/SSL backends
if exist "release\tls" (
    mkdir "%TEMP_DIR%\tls"
    xcopy "release\tls\*" "%TEMP_DIR%\tls\" /s /e /y >nul 2>&1
)

REM Copy Qt plugins - Network information
if exist "release\networkinformation" (
    mkdir "%TEMP_DIR%\networkinformation"
    xcopy "release\networkinformation\*" "%TEMP_DIR%\networkinformation\" /s /e /y >nul 2>&1
)

REM Copy Qt plugins - Generic
if exist "release\generic" (
    mkdir "%TEMP_DIR%\generic"
    xcopy "release\generic\*" "%TEMP_DIR%\generic\" /s /e /y >nul 2>&1
)

REM Copy Qt plugins - Icon engines
if exist "release\iconengines" (
    mkdir "%TEMP_DIR%\iconengines"
    xcopy "release\iconengines\*" "%TEMP_DIR%\iconengines\" /s /e /y >nul 2>&1
)

REM Copy Qt plugins - Styles
if exist "release\styles" (
    mkdir "%TEMP_DIR%\styles"
    xcopy "release\styles\*" "%TEMP_DIR%\styles\" /s /e /y >nul 2>&1
)

REM Create dist folder if it doesn't exist
if not exist "dist" mkdir "dist"

REM Create README for portable version
echo Creating README...
echo PlainMD v!VERSION! - Portable Windows Version>"%TEMP_DIR%\README-PORTABLE.txt"
echo.>>"%TEMP_DIR%\README-PORTABLE.txt"
echo This is a standalone portable version of PlainMD.>>"%TEMP_DIR%\README-PORTABLE.txt"
echo No installation required - just extract and run plainmd.exe>>"%TEMP_DIR%\README-PORTABLE.txt"
echo.>>"%TEMP_DIR%\README-PORTABLE.txt"
echo System Requirements:>>"%TEMP_DIR%\README-PORTABLE.txt"
echo - Windows 10 or later (64-bit)>>"%TEMP_DIR%\README-PORTABLE.txt"
echo - No additional dependencies needed>>"%TEMP_DIR%\README-PORTABLE.txt"
echo.>>"%TEMP_DIR%\README-PORTABLE.txt"
echo Usage:>>"%TEMP_DIR%\README-PORTABLE.txt"
echo 1. Extract this ZIP file to any folder>>"%TEMP_DIR%\README-PORTABLE.txt"
echo 2. Run plainmd.exe>>"%TEMP_DIR%\README-PORTABLE.txt"
echo 3. Optional: Pin to taskbar or create desktop shortcut>>"%TEMP_DIR%\README-PORTABLE.txt"
echo.>>"%TEMP_DIR%\README-PORTABLE.txt"
echo Features:>>"%TEMP_DIR%\README-PORTABLE.txt"
echo - Markdown Viewer with file tree sidebar>>"%TEMP_DIR%\README-PORTABLE.txt"
echo - Portable - runs from any location>>"%TEMP_DIR%\README-PORTABLE.txt"
echo - No registry entries or system modifications>>"%TEMP_DIR%\README-PORTABLE.txt"
echo - Settings and cached images stored in the application folder>>"%TEMP_DIR%\README-PORTABLE.txt"
echo.>>"%TEMP_DIR%\README-PORTABLE.txt"
echo For more information: https://github.com/BrainB0ne/PlainMD>>"%TEMP_DIR%\README-PORTABLE.txt"
echo.>>"%TEMP_DIR%\README-PORTABLE.txt"
echo Copyright (C) 2026 BrainByteZ>>"%TEMP_DIR%\README-PORTABLE.txt"
echo Licensed under GPLv3>>"%TEMP_DIR%\README-PORTABLE.txt"

REM Create portable marker file
 echo.>"%TEMP_DIR%\portable"

REM Create the ZIP file
echo.
echo ============================================
echo Creating ZIP archive...
echo ============================================

REM Remove old archive if exists
if exist "dist\!OUTPUT_FILE!" del /f "dist\!OUTPUT_FILE!"

REM Check if tar command is available (Windows 10+)
tar --version >nul 2>&1
if errorlevel 1 (
    echo ERROR: 'tar' command not found.
    echo.
    echo This script requires Windows 10 or later with built-in tar support.
    echo.
    echo Alternative: Manually zip the files from %TEMP_DIR%
    echo ============================================
    pause
    rmdir /s /q "%TEMP_DIR%"
    exit /b 1
)

REM Create zip archive using tar (change to temp dir and zip contents)
cd "%TEMP_DIR%"
tar -acf "..\dist\!OUTPUT_FILE!" *
cd ".."

if errorlevel 1 (
    echo ERROR: Failed to create archive.
    rmdir /s /q "%TEMP_DIR%"
    pause
    exit /b 1
)

REM Clean up temp directory
rmdir /s /q "%TEMP_DIR%"

echo.
echo ============================================
echo Portable ZIP created successfully!
echo ============================================
dir /b "dist\!OUTPUT_FILE!"
echo.
echo File location: dist\!OUTPUT_FILE!
echo.
echo Contents:
tar -tf "dist\!OUTPUT_FILE!" | findstr /v /c:"^!OUTPUT_FILE!$"
echo ============================================

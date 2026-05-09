@echo off
setlocal enabledelayedexpansion

:: PlainMD Windows Release Pipeline
:: Builds installer, portable ZIP, generates checksums, and archives everything.
:: Output: dist\plainmd-<version>-release.zip

set "SCRIPT_DIR=%~dp0"
cd /d "%SCRIPT_DIR%"

REM Extract version from src\main.cpp
set "VERSION="
for /f "delims=" %%a in ('findstr /r /c:"setApplicationVersion.*[0-9][0-9]*\.[0-9][0-9]*\.[0-9][0-9]*" src\main.cpp 2^>nul') do (
    for /f "tokens=2 delims=()" %%b in ("%%a") do (
        set "VER_TEMP=%%b"
        set "VERSION=!VER_TEMP:"=!"
    )
)

if "!VERSION!"=="" (
    echo Warning: Could not extract version from src\main.cpp
    set "VERSION=unknown"
)

echo ============================================
echo PlainMD Release Build
echo Version: !VERSION!
echo ============================================
echo.

:: ---------------------------------------------------------------------------
:: 1. Clean
:: ---------------------------------------------------------------------------
echo Step 1: Cleaning previous builds..
call clean.bat >nul 2>&1
if errorlevel 1 (
    echo   Warning: Clean step reported errors, continuing..
)

:: ---------------------------------------------------------------------------
:: 2. Build installer
:: ---------------------------------------------------------------------------
set "INNO_SETUP=C:\Program Files (x86)\Inno Setup 6\ISCC.exe"
if not exist "!INNO_SETUP!" (
    set "INNO_SETUP=C:\Program Files\Inno Setup 6\ISCC.exe"
)

if exist "!INNO_SETUP!" (
    echo.
    echo Step 2: Building installer
    call build-installer.bat
    if errorlevel 1 (
        echo   ERROR: Installer build failed.
    ) else (
        echo   Installer built successfully.
    )
) else (
    echo.
    echo Step 2: SKIPPED - Inno Setup not found.
    echo   Download from: https://jrsoftware.org/isdl.php
    echo.
    echo   Building binary only (no installer)
    call build.bat
    if errorlevel 1 (
        echo   ERROR: Build failed.
        pause
        exit /b 1
    )
)

:: ---------------------------------------------------------------------------
:: 3. Build portable ZIP
:: ---------------------------------------------------------------------------
if exist "release\plainmd.exe" (
    echo.
    echo Step 3: Building portable ZIP..
    call build-zip.bat
    if errorlevel 1 (
        echo   Warning: Portable ZIP build failed.
    )
) else (
    echo.
    echo Step 3: SKIPPED - release\plainmd.exe not found.
)

:: ---------------------------------------------------------------------------
:: 4. Generate checksums
:: ---------------------------------------------------------------------------
if exist "dist\" (
    echo.
    echo Step 4: Generating checksums..
    call make-checksums.bat
    if errorlevel 1 (
        echo   Warning: Checksum generation failed.
    )
) else (
    echo.
    echo Step 4: SKIPPED - dist\ folder not found.
)

:: ---------------------------------------------------------------------------
:: 5. Create release archive
:: ---------------------------------------------------------------------------
if exist "dist\" (
    echo.
    echo Step 5: Creating release archive..
    call archive-release.bat
    if errorlevel 1 (
        echo   Warning: Archive creation failed.
    )
) else (
    echo.
    echo Step 5: SKIPPED - nothing to archive.
)

echo.
echo ============================================
echo Release build complete!
echo ============================================
if exist "dist\" (
    echo.
    dir /b dist\
)
echo.
pause

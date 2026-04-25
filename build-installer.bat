@echo off
setlocal enabledelayedexpansion

:: Vibe-MD Windows Build and Installer Script
:: This script builds the application and creates an Inno Setup installer

set "SCRIPT_DIR=%~dp0"
cd /d "%SCRIPT_DIR%"

echo ============================================
echo Vibe-MD Build and Installer Script
echo ============================================
echo.

:: Check for Visual Studio 2022
set "VSVARS=C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvarsall.bat"
if not exist "%VSVARS%" (
    echo ERROR: Visual Studio 2022 not found at expected location.
    echo Please install VS2022 Community edition or update this script with your VS path.
    exit /b 1
)

:: Check for Inno Setup
set "INNO_SETUP=C:\Program Files (x86)\Inno Setup 6\ISCC.exe"
if not exist "%INNO_SETUP%" (
    set "INNO_SETUP=C:\Program Files\Inno Setup 6\ISCC.exe"
)
if not exist "%INNO_SETUP%" (
    echo ERROR: Inno Setup 6 not found.
    echo Please install Inno Setup from https://jrsoftware.org/isdl.php
    exit /b 1
)

echo Found Visual Studio 2022
echo Found Inno Setup
echo.

:: Setup MSVC environment
echo Setting up MSVC environment...
call "%VSVARS%" x64
if errorlevel 1 (
    echo ERROR: Failed to setup MSVC environment.
    exit /b 1
)

:: Check for Qt
echo Checking Qt installation...
qmake --version >nul 2>&1
if errorlevel 1 (
    echo ERROR: qmake not found in PATH.
    echo Please ensure Qt 6.x is installed and qmake is in your PATH.
    exit /b 1
)

:: Clean previous build
echo.
echo Cleaning previous build...
if exist release rmdir /s /q release
if exist debug rmdir /s /q debug
if exist Makefile del /q Makefile
if exist Makefile.Release del /q Makefile.Release
if exist Makefile.Debug del /q Makefile.Debug

:: Generate build files
echo.
echo Generating build files with qmake...
qmake vibe-md.pro
if errorlevel 1 (
    echo ERROR: qmake failed.
    exit /b 1
)

:: Build the project
echo.
echo Building Vibe-MD (Release)...
nmake
if errorlevel 1 (
    echo ERROR: Build failed.
    exit /b 1
)

:: Check executable was created
if not exist release\vibe-md.exe (
    echo ERROR: vibe-md.exe was not created.
    exit /b 1
)

echo.
echo Build successful!
echo.

:: Deploy Qt dependencies
echo Deploying Qt dependencies with windeployqt...
windeployqt release\vibe-md.exe
if errorlevel 1 (
    echo WARNING: windeployqt reported errors, continuing anyway...
)

echo.
echo Dependencies deployed.
echo.

:: Build installer
echo Building installer with Inno Setup...
"%INNO_SETUP%" installer.iss
if errorlevel 1 (
    echo ERROR: Inno Setup compiler failed.
    exit /b 1
)

echo.
echo ============================================
echo Build and Installer Creation Successful!
echo ============================================
echo.
echo Output: vibe-md-setup.exe
echo.

endlocal

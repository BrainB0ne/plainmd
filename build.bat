@echo off
setlocal enabledelayedexpansion

:: PlainMD Windows Build Script
:: Builds release binary and deploys Qt dependencies.
:: Output: release\plainmd.exe

set "SCRIPT_DIR=%~dp0"
cd /d "%SCRIPT_DIR%"

echo ============================================
echo PlainMD Build Script
echo ============================================
echo.

:: ---------------------------------------------------------------------------
:: 1. Locate Visual Studio 2022 vcvarsall.bat
:: ---------------------------------------------------------------------------
set "VSVARS="
for %%p in (Community Professional Enterprise) do (
    if not defined VSVARS (
        if exist "C:\Program Files\Microsoft Visual Studio\2022\%%p\VC\Auxiliary\Build\vcvarsall.bat" (
            set "VSVARS=C:\Program Files\Microsoft Visual Studio\2022\%%p\VC\Auxiliary\Build\vcvarsall.bat"
        )
    )
)

if not defined VSVARS (
    echo ERROR: Visual Studio 2022 not found at expected locations.
    echo Please install VS2022 or update this script with your installation path.
    pause
    exit /b 1
)

echo Found Visual Studio 2022
echo.

:: Setup MSVC environment
echo Setting up MSVC environment...
call "%VSVARS%" x64
if errorlevel 1 (
    echo ERROR: Failed to setup MSVC environment.
    pause
    exit /b 1
)

:: ---------------------------------------------------------------------------
:: 2. Check for Qt (qmake)
:: ---------------------------------------------------------------------------
echo Checking Qt installation...
qmake --version >nul 2>&1
if errorlevel 1 (
    echo ERROR: qmake not found in PATH.
    echo Please ensure Qt 6.x is installed and qmake is in your PATH.
    pause
    exit /b 1
)

:: ---------------------------------------------------------------------------
:: 3. Generate build files
:: ---------------------------------------------------------------------------
echo.
echo Generating build files with qmake...
qmake plainmd.pro
if errorlevel 1 (
    echo ERROR: qmake failed.
    pause
    exit /b 1
)

:: ---------------------------------------------------------------------------
:: 4. Build the project
:: ---------------------------------------------------------------------------
echo.
echo Building PlainMD (Release)...
nmake
if errorlevel 1 (
    echo ERROR: Build failed.
    pause
    exit /b 1
)

:: Check executable was created
if not exist "release\plainmd.exe" (
    echo ERROR: plainmd.exe was not created.
    pause
    exit /b 1
)

echo.
echo Build successful!
echo.

:: ---------------------------------------------------------------------------
:: 5. Deploy Qt dependencies
:: ---------------------------------------------------------------------------
echo Deploying Qt dependencies with windeployqt...
windeployqt release\plainmd.exe
if errorlevel 1 (
    echo WARNING: windeployqt reported errors, continuing anyway...
)

echo.
echo ============================================
echo Build Complete!
echo ============================================
echo.
echo Binary:  release\plainmd.exe
echo.
echo Run with:  release\plainmd.exe
echo.

endlocal

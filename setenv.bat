@echo off

:: PlainMD Environment Setup
:: Configures the MSVC x64 build environment for the current shell.
:: Run this before using qmake/nmake manually.

:: Check for Visual Studio 2022 (Community -> Professional -> Enterprise)
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
    echo.
    echo Searched:
    echo   C:\Program Files\Microsoft Visual Studio\2022\Community\...
    echo   C:\Program Files\Microsoft Visual Studio\2022\Professional\...
    echo   C:\Program Files\Microsoft Visual Studio\2022\Enterprise\...
    echo.
    echo Please install VS2022 or set a custom path in this script.
    pause
    exit /b 1
)

echo Setting up MSVC x64 environment...
call "%VSVARS%" x64
if errorlevel 1 (
    echo ERROR: Failed to setup MSVC environment.
    pause
    exit /b 1
)

echo Environment ready.
echo.
echo You can now run:
echo   qmake plainmd.pro
echo   nmake
echo.

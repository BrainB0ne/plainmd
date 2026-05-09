@echo off
setlocal EnableDelayedExpansion

REM Generate SHA256 checksums for distribution packages
REM Creates individual .sha256 files for each file in dist\

set "SCRIPT_DIR=%~dp0"
cd /d "%SCRIPT_DIR%"

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
    echo.
    echo Or create the dist\ folder manually and place
    echo your distribution files there.
    echo ============================================
    pause
    exit /b 1
)

cd dist

REM Check if dist folder has any files to checksum
set "HAS_FILES=0"
for %%f in (*.*) do (
    if /I not "%%~xf"==".sha256" (
        if /I not "%%~xf"==".sha256sum" (
            set "HAS_FILES=1"
        )
    )
)

if "%HAS_FILES%"=="0" (
    echo ============================================
    echo ERROR: No distribution files found in dist\
    echo ============================================
    echo.
    echo The dist\ folder exists but contains no files to checksum.
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

echo Generating SHA256 checksums for distribution packages...
echo.

REM Remove old checksum files
del /f /q *.sha256 2>nul
del /f /q SHA256SUMS 2>nul

REM Generate individual .sha256 files for each package
for %%f in (*.*) do (
    REM Skip directories and existing checksum files
    if /I not "%%~xf"==".sha256" (
        if /I not "%%~xf"==".sha256sum" (
            if /I not "%%~xf"==".tmp" (
                if "%%f" NEQ "SHA256SUMS" (
                    echo Processing: %%f
                    certutil -hashfile "%%f" SHA256 > "%%f.sha256.tmp" 2>nul
                    if errorlevel 1 (
                        echo   ERROR: Failed to generate hash for %%f
                        del "%%f.sha256.tmp" 2>nul
                    ) else (
                        REM Extract just the hash (certutil outputs extra lines)
                        set "HASH_LINE="
                        for /f "skip=1 tokens=*" %%a in (%%f.sha256.tmp) do (
                            if not defined HASH_LINE (
                                set "HASH_LINE=%%a"
                                echo %%a  %%f > "%%f.sha256"
                            )
                        )
                        del "%%f.sha256.tmp" 2>nul
                        echo   Created: %%f.sha256
                    )
                )
            )
        )
    )
)

REM Create combined SHA256SUMS file
echo.
echo Creating SHA256SUMS file...
> SHA256SUMS.tmp (
    for %%f in (*.*) do (
        if /I not "%%~xf"==".sha256" (
            if /I not "%%~xf"==".sha256sum" (
                if /I not "%%~xf"==".tmp" (
                    if "%%f" NEQ "SHA256SUMS" (
                        if exist "%%f.sha256" (
                            type "%%f.sha256"
                        )
                    )
                )
            )
        )
    )
)
move /y SHA256SUMS.tmp SHA256SUMS >nul 2>&1

cd ..

echo.
echo ============================================
echo Checksum files created successfully!
echo ============================================
dir /b dist/*.sha256 dist/SHA256SUMS 2>nul
echo.
echo To verify a single package:
echo   certutil -hashfile dist\package.exe SHA256
echo.
echo Or open the .sha256 file and compare values.
echo ============================================
exit /b 0

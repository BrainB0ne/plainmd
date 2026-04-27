@echo off
setlocal EnableDelayedExpansion

REM Generate SHA256 checksums for distribution packages
REM Creates individual .sha256 files for each file in dist\

set "SCRIPT_DIR=%~dp0"
cd /d "%SCRIPT_DIR%"

if not exist "dist\" (
    echo Error: dist\ folder not found.
    echo Build distribution packages first with:
    echo   build-installer.bat       # for Windows .exe
    echo   build-deb.sh              # for .deb (on Linux)
    echo   build-appimage.sh         # for .AppImage (on Linux)
    exit /b 1
)

echo Generating SHA256 checksums for distribution packages...

cd dist

REM Remove old checksum files
del /f /q *.sha256 2>nul
del /f /q SHA256SUMS 2>nul

REM Generate individual .sha256 files for each package
for %%f in (*.*) do (
    REM Skip directories and existing checksum files
    if /I not "%%~xf"==".sha256" (
        if /I not "%%~xf"==".sha256sum" (
            certutil -hashfile "%%f" SHA256 > "%%f.sha256.tmp"
            REM Extract just the hash and filename (certutil outputs extra lines)
            for /f "skip=1 tokens=*" %%a in (%%f.sha256.tmp) do (
                set "line=%%a"
                if not defined hash (
                    set "hash=%%a"
                    echo %%a  %%f > "%%f.sha256"
                )
            )
            set "hash="
            del "%%f.sha256.tmp" 2>nul
            echo   %%f.sha256
        )
    )
)

REM Also create a combined SHA256SUMS file using PowerShell
echo Creating SHA256SUMS...
powershell -Command "Get-ChildItem -File | Where-Object { $_.Extension -ne '.sha256' -and $_.Name -ne 'SHA256SUMS' } | ForEach-Object { $hash = (Get-FileHash $_.FullName -Algorithm SHA256).Hash.ToLower(); Write-Output \"$hash  $($_.Name)\" } | Out-File -FilePath 'SHA256SUMS' -Encoding ASCII"

cd ..

echo.
echo Checksum files created in dist\:
dir /b dist\*.sha256 dist\SHA256SUMS 2>nul
echo.
echo Verify a package with:
echo   certutil -hashfile dist\package.exe SHA256
echo   # Then compare with the value in dist\package.exe.sha256
echo.
echo Or verify all with PowerShell:
echo   cd dist ^&^& Get-Content SHA256SUMS ^| ForEach-Object { $parts = $_ -split '  ', 2; $expected = $parts[0]; $file = $parts[1]; $actual = (Get-FileHash $file -Algorithm SHA256).Hash.ToLower(); if ($expected -eq $actual) { Write-Host \"[OK] $file\" -ForegroundColor Green } else { Write-Host \"[FAIL] $file\" -ForegroundColor Red } }

@echo off

:: PlainMD Clean Script
:: Removes all build artifacts to return the repo to a clean state.

set "SCRIPT_DIR=%~dp0"
cd /d "%SCRIPT_DIR%"

echo Cleaning build artifacts...

:: Build directories
if exist "release" (
    rmdir /s /q "release" 2>nul
    if exist "release" (
        echo   Warning: Could not fully remove release\ (files may be in use)
    ) else (
        echo   Removed: release\
    )
)

if exist "debug" (
    rmdir /s /q "debug" 2>nul
    if exist "debug" (
        echo   Warning: Could not fully remove debug\ (files may be in use)
    ) else (
        echo   Removed: debug\
    )
)

:: Makefiles and qmake stash
for %%f in (Makefile Makefile.Release Makefile.Debug .qmake.stash) do (
    if exist "%%f" (
        del /q "%%f" 2>nul
        echo   Removed: %%f
    )
)

:: Qt generated files (fallback for root-level debris)
for %%p in (*.obj moc_*.cpp moc_*.h ui_*.h qrc_*.cpp) do (
    if exist "%%p" (
        del /q "%%p" 2>nul
        echo   Removed: %%p
    )
)

:: Backup files
for %%f in (*~) do (
    if exist "%%f" (
        del /q "%%f" 2>nul
        echo   Removed: %%f
    )
)

echo Clean complete!

@echo off
call "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvarsall.bat" x64
nmake clean 2>nul
if exist Makefile.Release (
    del /q Makefile Makefile.Release Makefile.Debug 2>nul
    echo Makefiles cleaned.
)
if exist release (
    rmdir /s /q release 2>nul
    echo Release folder cleaned.
)
echo Clean complete!

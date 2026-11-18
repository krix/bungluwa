@echo off
REM Build script for Bungluwa (Windows / MinGW GCC)
setlocal enabledelayedexpansion

set GCC=C:\mingw64\bin\gcc.exe
set OUTDIR=build
set OUT=%OUTDIR%\bungluwa.exe
set CFLAGS=-O2 -std=gnu11 -DRENDER_GL -DPLATFORM_SOKOL -Isrc -Ihigh_impact\src -Ihigh_impact\libs
set LDFLAGS=-lopengl32 -lkernel32 -luser32 -lshell32 -lgdi32 -lole32

REM Collect all .c files under src\
set SRCS=
for /r src %%f in (*.c) do set SRCS=!SRCS! "%%f"

REM Build
if not exist "%OUTDIR%" mkdir "%OUTDIR%"
echo Building bungluwa...
%GCC% -o "%OUT%" %CFLAGS% %SRCS% %LDFLAGS%
if %errorlevel% neq 0 (
    echo BUILD FAILED
    exit /b 1
)
echo BUILD OK: %OUT%

REM Copy assets
if exist "%OUTDIR%\assets" rmdir /s /q "%OUTDIR%\assets"
xcopy /s /e /q /i assets "%OUTDIR%\assets" >nul
echo Assets copied.
echo.
echo Run: %OUT%

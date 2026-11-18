@echo off
REM Build script for Bungluwa WASM (Emscripten)
REM Requires Emscripten SDK (emsdk) to be activated first:
REM   call D:\Projects\Copilot\emsdk\emsdk_env.bat
setlocal enabledelayedexpansion

set EMCC=emcc
set OUTDIR=build\wasm
set OUT=%OUTDIR%\game.js
set CFLAGS=-O2 -std=gnu11 -DRENDER_GL -DPLATFORM_SOKOL -Isrc -Ihigh_impact\src -Ihigh_impact\libs

REM Collect all .c files under src\ (excluding xinput_dynamic.c which is Windows-only)
set SRCS=
for /r src %%f in (*.c) do (
    echo %%f | findstr /i "xinput_dynamic" >nul
    if errorlevel 1 set SRCS=!SRCS! "%%f"
)

REM Build
if not exist "%OUTDIR%" mkdir "%OUTDIR%"

echo Building bungluwa for WASM...
%EMCC% %CFLAGS% %SRCS% -o "%OUT%" ^
    -lGL ^
    -s ALLOW_MEMORY_GROWTH=1 ^
    -s ENVIRONMENT=web ^
    -s MIN_WEBGL_VERSION=2 ^
    -s MAX_WEBGL_VERSION=2 ^
    -sEXPORTED_FUNCTIONS=_main,_input_set_button_state ^
    --preload-file assets@assets

if %errorlevel% neq 0 (
    echo BUILD FAILED
    exit /b 1
)

REM Copy HTML shell
copy /y src\wasm_index.html "%OUTDIR%\game.html" >nul

echo BUILD OK
echo.
echo Output: %OUTDIR%\game.html
echo To test, run a local web server in %OUTDIR%:
echo   cd %OUTDIR% ^&^& python -m http.server 8080
echo Then open http://localhost:8080/game.html

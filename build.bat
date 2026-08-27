@echo off
rem ============================================================
rem  HydraFW build script (Windows)
rem  Usage: build.bat [clean] [nopause]
rem  Output: src\build\hydrafw.bin / hydrafw.hex
rem ============================================================
setlocal enabledelayedexpansion

cd /d "%~dp0"
if errorlevel 1 goto :patherr
if not exist "src\Makefile" goto :patherr

set "PAUSE_ON=1"
set "MARGS="
:parseargs
if "%~1"=="" goto :argdone
if /i "%~1"=="nopause" (
    set "PAUSE_ON="
) else (
    set "MARGS=!MARGS! %~1"
)
shift
goto :parseargs
:argdone

rem --- Toolchain ---
set "GCC_OK="
if exist "C:\arm-eabi\bin\arm-none-eabi-gcc.exe" (
    set "PATH=C:\arm-eabi\bin;%PATH%"
    set "GCC_OK=1"
) else (
    where arm-none-eabi-gcc >nul 2>&1 && set "GCC_OK=1"
)
if not defined GCC_OK (
    echo [ERROR] arm-none-eabi-gcc not found.
    echo Install Arm GNU Toolchain or edit this file to set the correct path.
    goto :fail
)

rem --- Make ---
set "MAKE="
if exist "C:\ProgramData\chocolatey\bin\make.exe" (
    set "MAKE=C:\ProgramData\chocolatey\bin\make.exe"
) else (
    where make >nul 2>&1 && set "MAKE=make"
)
if not defined MAKE (
    echo [ERROR] make not found. Install GNU Make ^(choco install make^).
    goto :fail
)

echo === Building HydraFW ===
echo Make: %MAKE%
echo.

pushd "src"
"%MAKE%" -j4 !MARGS!
set RC=%ERRORLEVEL%
popd

if %RC% neq 0 (
    echo === Build FAILED ^(code %RC%^) ===
) else if exist "src\build\hydrafw.bin" (
    echo === Build OK: %~dp0src\build\hydrafw.bin ===
) else (
    echo === Make returned OK but hydrafw.bin missing ===
    set RC=1
)

if defined PAUSE_ON pause
exit /b %RC%

:patherr
echo [ERROR] Cannot find "%~dp0src\Makefile". Keep this .bat in the repo root.
goto :fail

:fail
if defined PAUSE_ON pause
exit /b 1

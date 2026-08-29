@echo off
:: This script compiles the project using GCC (MinGW-w64)

where gcc >nul 2>nul
if %errorlevel% neq 0 (
    echo [ERROR] 'gcc' not found in PATH.
    echo Please install MinGW-w64 and add it to your PATH.
    pause
    exit /b 1
)

echo Compiling OP_Auto.exe with GCC (Static)...

gcc -O2 -o OP_Auto.exe OP_Auto.c gui.c settings.c sat_prism.c self_update.c -I. -Ithird_party -mwindows -lgdi32 -lm -lpsapi -lurlmon -lbcrypt -lmsimg32

if %errorlevel% equ 0 (
    echo.
    echo [SUCCESS] OP_Auto.exe created successfully.
) else (
    echo.
    echo [FAILURE] Compilation failed.
)

pause
@echo off
echo Building OP_Auto GUI...

gcc -O2 -o OP_Auto.exe OP_Auto.c gui.c settings.c sat_prism.c self_update.c -I. -Ithird_party -mwindows -lgdi32 -lm -lpsapi -lurlmon -lbcrypt -lmsimg32

if %errorlevel% == 0 (
    echo Build successful! Created OP_Auto.exe
) else (
    echo Build failed with error code %errorlevel%
)

pause
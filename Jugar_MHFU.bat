@echo off
title Monster Hunter Freedom Unite - PC Native Port
cd /d "%~dp0"

echo ========================================================
echo   Monster Hunter Freedom Unite - Port Nativo PC
echo ========================================================
echo Iniciando juego con Vulkan y SDL3...
echo.

if exist "out\mhfu\bin\MHFU.exe" (
    start "" "out\mhfu\bin\MHFU.exe" "%~dp0profiles\mhfu\game"
) else if exist "MHFU.exe" (
    start "" "MHFU.exe" "%~dp0profiles\mhfu\game"
) else (
    echo Error: No se encontro MHFU.exe en out\mhfu\bin\
    pause
)

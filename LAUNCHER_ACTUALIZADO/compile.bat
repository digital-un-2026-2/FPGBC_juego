@echo off
setlocal

if defined GBDK (
    set "LCC_BIN=%GBDK%\bin\lcc.exe"
) else if defined GBDK_HOME (
    set "LCC_BIN=%GBDK_HOME%\bin\lcc.exe"
) else if exist "C:\gbdk\bin\lcc.exe" (
    set "LCC_BIN=C:\gbdk\bin\lcc.exe"
) else if exist "C:\gbdk-win64\gbdk\bin\lcc.exe" (
    set "LCC_BIN=C:\gbdk-win64\gbdk\bin\lcc.exe"
) else if exist "C:\gbdk-win32\gbdk\bin\lcc.exe" (
    set "LCC_BIN=C:\gbdk-win32\gbdk\bin\lcc.exe"
) else (
    set "LCC_BIN=lcc"
)

cd /d "%~dp0"

echo =========================================================================
echo Compilando FPGBC_LAUNCHER_ACTUALIZADO.gb con GBDK
echo Compilador: %LCC_BIN%
echo =========================================================================

"%LCC_BIN%" -DPLATFORM_GBDK -Wm-yn"FPGBC_GAME" -Wa-l -Wl-m -Wl-j -o FPGBC_LAUNCHER_ACTUALIZADO.gb main.c mario.c game.c tiles.c hal_gbdk.c

if %ERRORLEVEL% equ 0 (
    echo =========================================================================
    echo [EXITO] ROM generada correctamente: FPGBC_LAUNCHER_ACTUALIZADO.gb
    echo =========================================================================
) else (
    echo =========================================================================
    echo [ERROR] Fallo en la compilacion (Codigo %ERRORLEVEL%)
    echo =========================================================================
)

exit /b %ERRORLEVEL%

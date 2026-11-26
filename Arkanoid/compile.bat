@echo off
setlocal

if defined GBDK (
    set "LCC_BIN=%GBDK%\bin\lcc.exe"
) else if defined GBDK_HOME (
    set "LCC_BIN=%GBDK_HOME%\bin\lcc.exe"
) else if exist "C:\Users\ASUS\tools\gbdk-2020-4.5.0\gbdk\bin\lcc.exe" (
    set "LCC_BIN=C:\Users\ASUS\tools\gbdk-2020-4.5.0\gbdk\bin\lcc.exe"
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
echo Compilando arkanoid.gb con GBDK-2020
echo Compilador: %LCC_BIN%
echo =========================================================================

"%LCC_BIN%" -Wm-yc -Wm-yn"ARKANOID" -Wa-l -Wl-m -Wl-j -o arkanoid.gb main.c game.c levels.c sound.c tiles.c

if %ERRORLEVEL% equ 0 (
    echo =========================================================================
    echo [EXITO] ROM generada correctamente: arkanoid.gb
    echo =========================================================================
) else (
    echo =========================================================================
    echo [ERROR] Fallo en la compilacion (Codigo %ERRORLEVEL%)
    echo =========================================================================
)

exit /b %ERRORLEVEL%

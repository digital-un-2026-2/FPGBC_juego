@echo off
setlocal
if defined GBDK (
    set "GBDK_PATH=%GBDK%"
) else if defined GBDK_HOME (
    set "GBDK_PATH=%GBDK_HOME%"
) else if exist "C:\Users\ASUS\tools\gbdk-2020-4.5.0\gbdk\bin\lcc.exe" (
    set "GBDK_PATH=C:\Users\ASUS\tools\gbdk-2020-4.5.0\gbdk"
) else (
    echo [ERROR] No se encontro GBDK-2020. Por favor defina la variable GBDK o GBDK_HOME.
    exit /b 1
)

cd /d "%~dp0.."
if not exist build mkdir build

set "SRC=codigo\juego_completo.c"
set "OUT=build\juego_completo.gb"

echo =========================================================================
echo [COMPILANDO] %SRC% con GBDK-2020
echo [RUTA GBDK] %GBDK_PATH%
echo [DESTINO]   %OUT%
echo =========================================================================

"%GBDK_PATH%\bin\lcc.exe" -Wa-l -Wl-m -Wl-j -o "%OUT%" "%SRC%"
set "RC=%ERRORLEVEL%"

if %RC%==0 (
    echo [EXITO] ROM generada correctamente: %OUT%
) else (
    echo [ERROR] Fallo en la compilacion con codigo: %RC%
)
exit /b %RC%

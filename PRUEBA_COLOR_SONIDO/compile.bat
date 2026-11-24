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
echo Compilando ROMs de prueba (Color y Sonido) con GBDK
echo Compilador: %LCC_BIN%
echo =========================================================================

echo [1/2] Compilando test_color.gb...
"%LCC_BIN%" -DPLATFORM_GBDK -Wm-ynTEST_COLOR -Wm-yc -o test_color.gb test_color.c
if errorlevel 1 (
    echo [ERROR] Fallo al compilar test_color.gb
    exit /b 1
)
echo [OK] test_color.gb generado exitosamente.

echo.
echo [2/2] Compilando test_sonido.gb...
"%LCC_BIN%" -DPLATFORM_GBDK -Wm-ynTEST_SOUND -o test_sonido.gb test_sonido.c
if errorlevel 1 (
    echo [ERROR] Fallo al compilar test_sonido.gb
    exit /b 1
)
echo [OK] test_sonido.gb generado exitosamente.

echo.
echo =========================================================================
echo [EXITO] Ambas ROMs fueron compiladas correctamente:
echo   - test_color.gb
echo   - test_sonido.gb
echo =========================================================================

exit /b 0

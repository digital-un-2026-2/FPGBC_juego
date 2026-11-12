# Pong para FPGBC

Pong con sprites escrito en C, pensado para la consola FPGA **FPGBC**.
Mientras se prepara el hardware, el juego se prueba en el computador con un
simulador hecho con [raylib](https://www.raylib.com/).

## Cómo funciona

El juego está separado en dos partes:

- **El juego** (`game.c`, `tiles.c`): lógica, reglas y gráficos. Solo usa
  enteros y `<stdint.h>`, sin `float`, `malloc` ni `printf`, para que compile
  igual en el PC y en el procesador de la FPGA.
- **El hardware** (`hal.h`): lista de funciones que el juego necesita
  (cargar tiles, poner sprites, leer botones, esperar el VBlank).
  - En el PC las implementa `hal_pc.c` con raylib.
  - En la FPGBC se implementarán escribiendo en los registros de los
    periféricos de la consola.

El juego no llama a raylib directamente. Raylib solo se usa para abrir la
ventana, leer el teclado y mostrar una imagen de 160x144. Los tiles, el fondo
y los sprites se dibujan "a mano" en `hal_pc.c`, como lo haría el periférico
de video.

## Archivos

| Archivo | Para qué sirve |
|---------|----------------|
| `main.c` | Punto de entrada: inicia el hardware y el juego, y repite cada cuadro |
| `game.c`, `game.h` | Lógica del Pong (raquetas, bola, marcador, estados) |
| `tiles.c`, `tiles.h` | Gráficos del juego (tiles de 8x8, 2 bits por píxel) |
| `hal.h` | Interfaz de hardware que usa el juego |
| `hal_pc.c` | Simulador en PC con raylib |
| `hal_fpgbc.c` | HAL de la consola: botones reales, video según `VIDEO.md`, depuración por UART |
| `VIDEO.md` | Propuesta de periférico de video para acordar con el grupo de video |
| `Makefile` | Compila el simulador en Windows (`make`) y corre las pruebas (`make test`) |
| `Makefile.fpgbc` | Compila `pong.bin` para la consola con el toolchain de LiteX |
| `pruebas/hal_test.c` | Pruebas automáticas sin ventana: menú, pausa, fin de partida, revancha |

## Requisitos para compilarlo en Windows

Necesitas dos cosas: un compilador de C (w64devkit) y la librería raylib.

### 1. Instalar w64devkit (compilador y `make`)

1. Descarga la última versión desde
   <https://github.com/skeeto/w64devkit/releases> (archivo `w64devkit-x.x.x.zip`
   o el `.exe` autoextraíble).
2. Descomprímela en una carpeta fija, por ejemplo `C:\w64devkit`.
3. Para usar `gcc` y `make` desde cualquier terminal (PowerShell o la terminal de
   VSCode), agrega `C:\w64devkit\bin` a la variable de entorno **Path**.
   Cierra y vuelve a abrir la terminal después.
4. Comprueba que funciona:

   ```powershell
   gcc --version
   make --version
   ```

   Si no quieres tocar el Path, también puedes abrir `w64devkit.exe`, que
   abre una terminal con todo listo.

### 2. Descargar raylib

1. Ve a <https://github.com/raysan5/raylib/releases> y descarga la última
   versión para Windows con MinGW-w64. El archivo se llama parecido a
   `raylib-X.Y.Z_win64_mingw-w64.zip`.
2. Descomprímelo. Dentro hay una carpeta `include` (con `raylib.h`) y una
   carpeta `lib` (con `libraylib.a`).
3. **Copia las carpetas `include` y `lib` junto al `Makefile`**, dentro de la
   carpeta `pong`. Así el `Makefile` las encuentra sin cambiar nada
   (`RAYLIB = .`).

La carpeta debería verse así:

```
pong/
├── include/        <- de raylib
├── lib/            <- de raylib
├── Makefile
├── main.c
├── game.c
└── ...
```

> Las carpetas `include` y `lib` de raylib no están en este repositorio:
> hay que descargarlas con el paso anterior.

**Si instalaste raylib en otra carpeta** (por ejemplo con el instalador oficial
en `C:/raylib/raylib`), no copies nada y cambia esta línea del `Makefile`:

```make
RAYLIB = C:/raylib/raylib
```

## Compilar y ejecutar

Desde una terminal, dentro de la carpeta `pong`:

```powershell
make
.\pong.exe
```

El `make` genera `pong.exe` a partir de `main.c`, `game.c`, `tiles.c` y
`hal_pc.c`. Para borrar el ejecutable:

```powershell
make clean
```

El ejecutable `pong.exe` ya no se sube al repositorio (está en `.gitignore`);
cada quien lo genera con `make`.

## Pruebas automáticas

No necesitan raylib. Desde la carpeta `pong`:

```powershell
make test
```

Recorren el menú, las tres dificultades, la pausa, una partida ganada (un
"bot" sigue la bola), una perdida y la revancha. Dejan capturas `.ppm` en
`pruebas/capturas/`.

## Controles

| Acción | Botón de la consola | Teclado (simulador) |
|--------|-----|--------|
| Elegir dificultad en el menú | Arriba / Abajo | Flechas o `W` / `S` |
| Empezar | START o A | `Enter` o `Z` |
| Mover la raqueta | Arriba / Abajo | Flechas o `W` / `S` |
| Sacar la bola | A o START | `Z` o `Enter` |
| Pausa / continuar | START | `Enter` |
| Volver al menú (en pausa o al terminar) | SELECT (o B al terminar) | `Shift` derecho (o `X`) |
| Revancha al terminar | START | `Enter` |

Juegas con la raqueta de la izquierda contra la CPU. Gana quien llegue
primero a 7 puntos. Cada 4 golpes seguidos la bola aumenta de velocidad (y
parpadea para avisarlo), y el ángulo de rebote depende de en qué parte de la
raqueta pegue. La raqueta tiembla un instante al golpear.

| Dificultad | CPU | Bola máxima |
|---|---|---|
| Fácil | Lenta, se mueve 1 de cada 2 cuadros y no apunta | 3 px/cuadro |
| Normal | Como la versión original: apunta a las esquinas y se esfuerza si va perdiendo | 4 px/cuadro |
| Difícil | Doble de rápida, siempre apunta | 5 px/cuadro |

## Problemas frecuentes

| Mensaje o problema | Qué hacer |
|--------------------|-----------|
| `'make' no se reconoce como nombre de un cmdlet` | w64devkit no está en el Path. Agrega `C:\w64devkit\bin` y reabre la terminal, o usa `w64devkit.exe` |
| `fatal error: raylib.h: No such file or directory` | Falta la carpeta `include` de raylib junto al `Makefile`, o `RAYLIB` apunta a una ruta incorrecta |
| `cannot find -lraylib` | Falta `libraylib.a` en la carpeta `lib`, o la versión descargada no es la de MinGW-w64 |
| La ventana no abre al ejecutar `pong.exe` | Compílalo de nuevo en tu equipo con `make` |

## En la consola FPGBC

`hal_fpgbc.c` ya implementa `hal.h` para el SoC de `FPGBC_digital_UN`:

- **Botones:** lee el periférico `game_buttons` (hay que construir el SoC con
  `--with-game-buttons`). Invierte los bits (son activos en bajo) y los
  reordena al formato de `hal.h`.
- **Video:** todavía no existe el periférico. `VIDEO.md` propone su mapa de
  registros; cuando el SoC tenga una región `video`, el juego la usa sola.
- **Mientras tanto:** compilando con `UART_DEBUG=1`, el juego dibuja la pantalla
  con caracteres por el UART (`#` raquetas, `o` bola, `|` red), 4 veces por
  segundo. Así se puede jugar en la FPGA con `litex_term` antes de tener video.

```bash
make -f Makefile.fpgbc UART_DEBUG=1
litex_term /dev/ttyUSB0 --kernel=pong.bin
```

`Makefile.fpgbc` no se ha podido probar sin el build del SoC; revisar las rutas
`BUILD_DIR` y `LINKER` al primer intento.

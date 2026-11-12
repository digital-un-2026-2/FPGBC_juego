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

## Archivos modificados respecto a daricor

| Archivo | Estado | Qué cambió |
|---|---|---|
| `game.c` | Modificado | Pantalla de título y menú, tabla de 3 dificultades, pausa, GANASTE/PERDISTE, revancha y vuelta al menú, temblor de raqueta, parpadeo de la bola, arreglo de la bola horizontal, comentario de la IA corregido. Se conserva la IA de David como nivel Normal |
| `tiles.c` | Modificado | 26 letras A–Z en el mismo estilo que los dígitos y una flecha para el menú (42 tiles en total, el límite es 64) |
| `tiles.h` | Modificado | Nuevos índices `TILE_LETTER_A` y `TILE_CURSOR` |
| `hal_pc.c` | Modificado | Define `PANEL_SIZE` (640 px) si no viene definido; antes no compilaba |
| `Makefile` | Modificado | Nuevo objetivo `make test` y variable `CFLAGS_EXTRA` |
| `README.md` | Modificado | Controles nuevos, tabla de dificultades, pruebas y cómo correrlo en la consola |
| `hal_fpgbc.c` | Nuevo | HAL de la consola: botones reales, video según `VIDEO.md` y depuración por UART |
| `Makefile.fpgbc` | Nuevo | Compila `pong.bin` con el toolchain de LiteX, basado en el ejemplo de botones |
| `VIDEO.md` | Nuevo | Propuesta de mapa de registros del periférico de video |
| `pruebas/hal_test.c` | Nuevo | 19 pruebas automáticas sin ventana |
| `pruebas/capturas/*.png` | Nuevo | 5 capturas generadas por las pruebas |
| `.gitignore` | Nuevo | Ignora `pong.exe`, las carpetas de raylib, binarios y capturas `.ppm` |
| `hal.h`, `main.c`, `game.h` | Sin cambios | Se quedan como los dejó David |
| `pong.exe` | Se quita del repo | Se genera con `make`; pesaba 1,2 MB | |


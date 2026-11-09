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
| `Makefile` | Compila el juego en Windows |
| `pong.exe` | Ejecutable ya compilado para Windows |

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

Si solo quieres jugar sin compilar, puedes abrir directamente el `pong.exe`
incluido en esta carpeta.

## Controles

| Acción | Teclas |
|--------|--------|
| Mover la raqueta | Flechas arriba/abajo, o `W` / `S` |
| Sacar la bola | `Enter` o `Z` |
| Reiniciar al terminar la partida | `Enter` |

Juegas con la raqueta de la izquierda contra la CPU. Gana quien llegue
primero a 7 puntos. Cada 4 golpes seguidos la bola aumenta de velocidad, y el
ángulo de rebote depende de en qué parte de la raqueta pegue.

## Problemas frecuentes

| Mensaje o problema | Qué hacer |
|--------------------|-----------|
| `'make' no se reconoce como nombre de un cmdlet` | w64devkit no está en el Path. Agrega `C:\w64devkit\bin` y reabre la terminal, o usa `w64devkit.exe` |
| `fatal error: raylib.h: No such file or directory` | Falta la carpeta `include` de raylib junto al `Makefile`, o `RAYLIB` apunta a una ruta incorrecta |
| `cannot find -lraylib` | Falta `libraylib.a` en la carpeta `lib`, o la versión descargada no es la de MinGW-w64 |
| La ventana no abre al ejecutar `pong.exe` | Compílalo de nuevo en tu equipo con `make` |

## Próximo paso

Escribir `hal_fpgbc.c`, que implemente las funciones de `hal.h` con los
periféricos reales de la FPGBC (memoria de tiles, OAM, joypad y VBlank), para
grabar el juego en el cartucho. `game.c` y `tiles.c` no deberían necesitar
cambios.

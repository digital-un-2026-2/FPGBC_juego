# Propuesta: periférico de video para la FPGBC

Este documento es una **propuesta para acordar con el grupo de video**. Describe
el periférico mínimo que necesitan los juegos escritos sobre `hal.h` (Pong y,
más adelante, el juego de plataformas). `hal_fpgbc.c` ya está escrito contra
este mapa de registros: si el SoC expone una región llamada `video`, LiteX
genera `VIDEO_BASE` en `generated/mem.h` y el juego la usa sin cambios.

Nada de esto está implementado todavía en `FPGBC_digital_UN`.

## Qué necesita el juego

| Función de `hal.h` | Qué hace en el hardware |
|---|---|
| `hal_load_tiles(first, count, data)` | Escribe `count × 16` bytes en la memoria de tiles |
| `hal_set_bg(tx, ty, tile)` | Escribe un byte en el mapa de fondo |
| `hal_set_sprite(id, x, y, tile)` / `hal_hide_sprite(id)` | Escribe una palabra de la OAM |
| `hal_frame()` | Espera la bandera de VBlank y la borra |
| `hal_buttons()` | Ya existe: periférico `game_buttons` |

## Formato de imagen

- Pantalla lógica de **160 × 144** píxeles, 4 tonos (2 bits por píxel).
- Tiles de **8 × 8**, formato Game Boy: 16 bytes por tile, 2 bytes por fila
  (byte par = bit 0 del color, byte impar = bit 1), bit 7 = píxel de la izquierda.
- En sprites el color 0 es transparente.
- Prioridad: el sprite con id más bajo queda encima.

## Mapa de registros (bus Wishbone de 32 bits con byte-enable)

Direcciones relativas a `VIDEO_BASE`.

| Offset | Tamaño | Nombre | Descripción |
|---|---|---|---|
| `0x0000` | 1024 B | `TILE_RAM` | 64 tiles × 16 bytes. Escritura por byte |
| `0x0400` | 1024 B | `BG_MAP` | Mapa de 32 × 32 bytes, celda = `fila * 32 + columna`. Se ven 20 × 18 celdas a partir de (`SCX`, `SCY`) |
| `0x0800` | 64 B | `OAM` | 16 sprites, una palabra de 32 bits cada uno (ver abajo) |
| `0x0C00` | 4 B | `CTRL` | bit 0 = fondo encendido, bit 1 = sprites encendidos |
| `0x0C04` | 4 B | `STATUS` | bit 0 = "estoy en VBlank" (solo lectura); bit 1 = bandera de VBlank: se pone en 1 al empezar cada VBlank y se borra escribiendo 1 |
| `0x0C08` | 4 B | `SCX` | Desplazamiento horizontal del fondo en píxeles (0 para el Pong) |
| `0x0C0C` | 4 B | `SCY` | Desplazamiento vertical del fondo en píxeles (0 para el Pong) |

### Palabra de la OAM

| Bits | Campo | Nota |
|---|---|---|
| 7..0 | `Y + 16` | Como en la Game Boy, permite sprites que salen por arriba |
| 15..8 | `X + 8` | Permite sprites que salen por la izquierda |
| 23..16 | `TILE` | 0..63 |
| 31..24 | `ATTR` | bit 7 = visible. Reservados para después: bit 5 = espejo horizontal, bit 6 = espejo vertical |

## Recursos estimados

- Memoria: 1 KB de tiles + 1 KB de mapa + 64 B de OAM, cabe en 2 bloques BSRAM
  de la GW2AR-18 (tiene 46).
- Por línea se comparan como máximo 16 sprites; el Pong usa 9.
- `SCX`/`SCY` no los usa el Pong, pero los necesita el juego de plataformas
  (mapa de 96 columnas con scroll).

## Preguntas abiertas para el grupo de video

1. ¿La pantalla de la consola es 160 × 144 o se escala a la del LCD real?
2. ¿El periférico lee las memorias con doble puerto (CPU escribe mientras se dibuja)?
   Si no, la CPU solo debería escribir durante el VBlank.
3. ¿Se puede conectar la bandera de VBlank a una interrupción del FemtoRV?

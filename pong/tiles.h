#ifndef TILES_H
#define TILES_H

#include <stdint.h>

/* Indices de tile: el orden DEBE coincidir con tiles.c */
enum {
    TILE_EMPTY = 0,
    TILE_SOLID,
    TILE_WALL,
    TILE_BALL,
    TILE_NET,
    TILE_DIGIT0,                      /* digitos 0..9 -> TILE_DIGIT0 + n */
    TILE_LETTER_A = TILE_DIGIT0 + 10, /* letras A..Z -> TILE_LETTER_A + (letra - 'A') */
    TILE_CURSOR   = TILE_LETTER_A + 26,
    TILE_COUNT                        /* 42 tiles (el HAL admite hasta MAX_TILES = 64) */
};

extern const uint8_t game_tiles[TILE_COUNT * 16];

#endif

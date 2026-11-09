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
    TILE_COUNT = TILE_DIGIT0 + 10
};

extern const uint8_t game_tiles[TILE_COUNT * 16];

#endif

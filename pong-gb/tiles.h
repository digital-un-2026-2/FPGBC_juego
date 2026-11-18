#ifndef TILES_H
#define TILES_H

#include <stdint.h>

enum {
    TILE_EMPTY = 0,
    TILE_SOLID,
    TILE_WALL,
    TILE_BALL,
    TILE_NET,
    TILE_DIGIT0,
    TILE_LETTER_A = TILE_DIGIT0 + 10,
    TILE_CURSOR   = TILE_LETTER_A + 26,
    TILE_COUNT
};

extern const uint8_t game_tiles[TILE_COUNT * 16];

#endif
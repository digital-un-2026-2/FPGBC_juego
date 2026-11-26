#ifndef TILES_H
#define TILES_H

#include <gb/gb.h>
#include <gb/cgb.h>
#include <stdint.h>

/* Paletas CGB de fondo (BG 0..7) */
enum {
    PAL_UI = 0,
    PAL_WALL,
    PAL_RED,
    PAL_ORANGE,
    PAL_YELLOW,
    PAL_GREEN,
    PAL_CYAN,
    PAL_SILVER
};

/* Paletas CGB de sprites (SPR 0..2) */
enum {
    PAL_SPR_PADDLE = 0,
    PAL_SPR_BALL,
    PAL_SPR_FX
};

/* Indices de tiles de fondo (0..55) */
enum {
    TILE_EMPTY = 0,
    TILE_BG_FIELD,
    TILE_WALL_TL,
    TILE_WALL_T,
    TILE_WALL_TR,
    TILE_WALL_L,
    TILE_WALL_R,
    TILE_BRICK1_L,
    TILE_BRICK1_R,
    TILE_BRICK2_L,
    TILE_BRICK2_R,
    TILE_BRICK3_L,
    TILE_BRICK3_R,
    TILE_BRICK_CRK_L,
    TILE_BRICK_CRK_R,
    TILE_HEART,
    TILE_CURSOR,
    TILE_COLON,
    TILE_EXCL,
    TILE_DASH,
    TILE_DIGIT0,       /* 20..29: '0'..'9' */
    TILE_LETTER_A = TILE_DIGIT0 + 10, /* 30..55: 'A'..'Z' */
    BG_TILE_COUNT = TILE_LETTER_A + 26
};

/* Indices de tiles de sprites en VRAM 0x8000 (despues de BG_TILE_COUNT para evitar solapamiento) */
enum {
    SPR_TILE_PAD_L = BG_TILE_COUNT,
    SPR_TILE_PAD_M,
    SPR_TILE_PAD_R,
    SPR_TILE_BALL,
    SPR_TILE_FX0,
    SPR_TILE_FX1,
    SPR_TILE_COUNT = 6
};

extern const uint8_t bg_tiles[BG_TILE_COUNT * 16];
extern const uint8_t spr_tiles[SPR_TILE_COUNT * 16];
extern const palette_color_t bg_palettes[8 * 4];
extern const palette_color_t spr_palettes[3 * 4];

void gfx_init(void);
void gfx_set_tile(uint8_t tx, uint8_t ty, uint8_t tile, uint8_t pal);
void gfx_clear_screen(uint8_t tile, uint8_t pal);
uint8_t gfx_char_to_tile(char c);
void gfx_draw_text(uint8_t tx, uint8_t ty, const char *str, uint8_t pal);
void gfx_draw_number(uint8_t tx, uint8_t ty, uint16_t val, uint8_t digits, uint8_t pal);

#endif

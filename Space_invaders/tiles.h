#ifndef TILES_H
#define TILES_H

#include <gb/gb.h>
#include <gb/cgb.h>
#include <stdint.h>

/* Paletas CGB de fondo (BG 0..7) */
enum {
    PAL_BG_UI = 0,
    PAL_BG_YELLOW,
    PAL_BG_CYAN,
    PAL_BG_GREEN,
    PAL_BG_RED,
    PAL_BG_ORANGE,
    PAL_BG_MAGENTA,
    PAL_BG_GRAY
};

/* Paletas CGB de sprites (SPR 0..5) */
enum {
    PAL_SPR_PLAYER = 0,
    PAL_SPR_GREEN,
    PAL_SPR_CYAN,
    PAL_SPR_YELLOW,
    PAL_SPR_RED,
    PAL_SPR_BOSS
};

/* Tiles de fondo en VRAM 0x9000 (modo LCDCF_BG8800) */
enum {
    TILE_EMPTY = 0,
    TILE_STAR1,
    TILE_STAR2,
    TILE_SHD_TL,
    TILE_SHD_TR,
    TILE_SHD_BL,
    TILE_SHD_BR,
    TILE_SHD_DMG1,
    TILE_SHD_DMG2,
    TILE_LIFE,
    TILE_BAR_EMPTY,
    TILE_BAR_FULL,
    TILE_CURSOR,
    TILE_COLON,
    TILE_EXCL,
    TILE_DASH,
    TILE_EQ,
    TILE_SLASH,
    TILE_DIGIT0,       /* 18..27: '0'..'9' */
    TILE_LETTER_A = TILE_DIGIT0 + 10, /* 28..53: 'A'..'Z' */
    BG_TILE_COUNT = TILE_LETTER_A + 26
};

/* Tiles de sprites en VRAM 0x8000 (separados de BG en modo LCDCF_BG8800) */
enum {
    SPR_SHIP_L = 0,
    SPR_SHIP_R,
    SPR_SHIP_FIRE_L,
    SPR_SHIP_FIRE_R,
    SPR_GRUNT_0,
    SPR_GRUNT_1,
    SPR_SHOOT_0,
    SPR_SHOOT_1,
    SPR_ARMOR_0,
    SPR_ARMOR_1,
    SPR_DIVER_0,
    SPR_DIVER_1,
    SPR_UFO_L,
    SPR_UFO_R,
    SPR_BOSS_TL,
    SPR_BOSS_TM,
    SPR_BOSS_TR,
    SPR_BOSS_BL,
    SPR_BOSS_CORE,
    SPR_BOSS_BR,
    SPR_PSHOT,
    SPR_SPSHOT,
    SPR_ESHOT_0,
    SPR_ESHOT_1,
    SPR_EXP_0,
    SPR_EXP_1,
    SPR_TILE_COUNT
};

extern const uint8_t bg_tiles[BG_TILE_COUNT * 16];
extern const uint8_t spr_tiles[SPR_TILE_COUNT * 16];
extern const palette_color_t bg_palettes[8 * 4];
extern const palette_color_t spr_palettes[6 * 4];

void gfx_init(void);
void gfx_set_tile(uint8_t tx, uint8_t ty, uint8_t tile, uint8_t pal);
void gfx_clear_screen(uint8_t tile, uint8_t pal);
uint8_t gfx_star_tile(uint8_t tx, uint8_t ty);
void gfx_draw_starfield(uint8_t start_ty, uint8_t end_ty);
void gfx_draw_text(uint8_t tx, uint8_t ty, const char *str, uint8_t pal);
void gfx_draw_number(uint8_t tx, uint8_t ty, uint16_t val, uint8_t digits, uint8_t pal);

#endif

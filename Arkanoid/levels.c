#include "levels.h"
#include "tiles.h"

uint8_t brick_hp[BRICK_ROWS][BRICK_COLS];
uint8_t brick_type[BRICK_ROWS][BRICK_COLS];
uint8_t g_bricks_left;

static const char *const level_names[NUM_LEVELS] = {
    "CLASICO",
    "FORTALEZA",
    "CIUDADELA"
};

/* Plantillas de niveles: 0=vacio, 1..5=1HP colores, 6=2HP reforzado, 7=3HP blindado */
static const uint8_t level_maps[NUM_LEVELS][BRICK_ROWS][BRICK_COLS] = {
    /* Nivel 1: CLASICO (4 filas completas = 36 bloques) */
    {
        { 1, 1, 1, 1, 1, 1, 1, 1, 1 },
        { 2, 2, 2, 2, 2, 2, 2, 2, 2 },
        { 3, 3, 3, 3, 3, 3, 3, 3, 3 },
        { 4, 4, 4, 4, 4, 4, 4, 4, 4 },
        { 0, 0, 0, 0, 0, 0, 0, 0, 0 },
        { 0, 0, 0, 0, 0, 0, 0, 0, 0 },
        { 0, 0, 0, 0, 0, 0, 0, 0, 0 }
    },
    /* Nivel 2: FORTALEZA (pasillos laterales + nucleo reforzado = 40 bloques) */
    {
        { 1, 1, 1, 1, 1, 1, 1, 1, 1 },
        { 2, 0, 2, 2, 2, 2, 2, 0, 2 },
        { 3, 0, 3, 6, 6, 6, 3, 0, 3 },
        { 4, 0, 4, 3, 3, 3, 4, 0, 4 },
        { 5, 5, 5, 0, 0, 0, 5, 5, 5 },
        { 0, 6, 0, 6, 0, 6, 0, 6, 0 },
        { 0, 0, 0, 0, 0, 0, 0, 0, 0 }
    },
    /* Nivel 3: CIUDADELA (bloques blindados 3HP y reforzados 2HP = 47 bloques) */
    {
        { 7, 1, 1, 1, 7, 1, 1, 1, 7 },
        { 1, 2, 0, 2, 6, 2, 0, 2, 1 },
        { 2, 6, 3, 3, 1, 3, 3, 6, 2 },
        { 0, 3, 7, 4, 4, 4, 7, 3, 0 },
        { 4, 4, 0, 6, 6, 6, 0, 4, 4 },
        { 5, 0, 6, 0, 0, 0, 6, 0, 5 },
        { 0, 6, 0, 7, 0, 7, 0, 6, 0 }
    }
};

struct BrickInfo {
    uint8_t  init_hp;
    uint8_t  pal;
    uint16_t hit_pts;
    uint16_t break_pts;
};

static const struct BrickInfo brick_table[8] = {
    { 0U, PAL_UI,     0U,   0U   }, /* 0: vacio */
    { 1U, PAL_RED,    0U,   100U }, /* 1: rojo */
    { 1U, PAL_ORANGE, 0U,   80U  }, /* 2: naranja */
    { 1U, PAL_YELLOW, 0U,   60U  }, /* 3: amarillo */
    { 1U, PAL_GREEN,  0U,   50U  }, /* 4: verde */
    { 1U, PAL_CYAN,   0U,   40U  }, /* 5: cian */
    { 2U, PAL_CYAN,   20U,  150U }, /* 6: reforzado 2 HP */
    { 3U, PAL_SILVER, 25U,  250U }  /* 7: blindado 3 HP */
};

void level_load(uint8_t level_idx)
{
    uint8_t r, c;
    if (level_idx >= NUM_LEVELS) level_idx = 0U;

    g_bricks_left = 0U;
    for (r = 0U; r < BRICK_ROWS; r++) {
        for (c = 0U; c < BRICK_COLS; c++) {
            uint8_t t = level_maps[level_idx][r][c];
            brick_type[r][c] = t;
            brick_hp[r][c] = brick_table[t].init_hp;
            if (brick_hp[r][c] > 0U) {
                g_bricks_left++;
            }
        }
    }
}

const char *level_get_name(uint8_t level_idx)
{
    if (level_idx >= NUM_LEVELS) level_idx = 0U;
    return level_names[level_idx];
}

void level_draw_brick(uint8_t row, uint8_t col)
{
    uint8_t hp = brick_hp[row][col];
    uint8_t t = brick_type[row][col];
    uint8_t tx = (uint8_t)(1U + (col << 1));
    uint8_t ty = (uint8_t)(BRICK_TOP_TY + row);
    uint8_t tile_l, tile_r, pal;

    if (hp == 0U || t == 0U) {
        gfx_set_tile(tx, ty, TILE_BG_FIELD, PAL_UI);
        gfx_set_tile((uint8_t)(tx + 1U), ty, TILE_BG_FIELD, PAL_UI);
        return;
    }

    pal = brick_table[t].pal;
    if (hp >= 3U) {
        tile_l = TILE_BRICK3_L;
        tile_r = TILE_BRICK3_R;
    } else if (hp == 2U) {
        tile_l = TILE_BRICK2_L;
        tile_r = TILE_BRICK2_R;
    } else if (t >= 6U) {
        tile_l = TILE_BRICK_CRK_L;
        tile_r = TILE_BRICK_CRK_R;
    } else {
        tile_l = TILE_BRICK1_L;
        tile_r = TILE_BRICK1_R;
    }

    gfx_set_tile(tx, ty, tile_l, pal);
    gfx_set_tile((uint8_t)(tx + 1U), ty, tile_r, pal);
}

void level_draw_playfield(void)
{
    uint8_t r, c;

    /* Limpiar filas 0..17 con fondo base */
    gfx_clear_screen(TILE_EMPTY, PAL_UI);

    /* Fila 1: muro superior metalico y esquinas */
    gfx_set_tile(0U, 1U, TILE_WALL_TL, PAL_WALL);
    fill_bkg_rect(1U, 1U, 18U, 1U, TILE_WALL_T);
    VBK_REG = 1;
    fill_bkg_rect(1U, 1U, 18U, 1U, PAL_WALL);
    VBK_REG = 0;
    gfx_set_tile(19U, 1U, TILE_WALL_TR, PAL_WALL);

    /* Filas 2..17: muros laterales y fondo de cancha */
    fill_bkg_rect(0U, 2U, 1U, 16U, TILE_WALL_L);
    fill_bkg_rect(19U, 2U, 1U, 16U, TILE_WALL_R);
    fill_bkg_rect(1U, 2U, 18U, 16U, TILE_BG_FIELD);
    VBK_REG = 1;
    fill_bkg_rect(0U, 2U, 1U, 16U, PAL_WALL);
    fill_bkg_rect(19U, 2U, 1U, 16U, PAL_WALL);
    fill_bkg_rect(1U, 2U, 18U, 16U, PAL_UI);
    VBK_REG = 0;

    /* Dibujar bloques activos */
    for (r = 0U; r < BRICK_ROWS; r++) {
        for (c = 0U; c < BRICK_COLS; c++) {
            if (brick_hp[r][c] > 0U) {
                level_draw_brick(r, c);
            }
        }
    }
}

uint16_t level_hit_brick(uint8_t row, uint8_t col, uint8_t *destroyed)
{
    uint8_t hp = brick_hp[row][col];
    uint8_t t = brick_type[row][col];
    uint16_t pts = 0U;

    *destroyed = 0U;
    if (hp == 0U || t == 0U) return 0U;

    hp--;
    brick_hp[row][col] = hp;
    if (hp == 0U) {
        if (g_bricks_left > 0U) g_bricks_left--;
        *destroyed = 1U;
        pts = brick_table[t].break_pts;
    } else {
        pts = brick_table[t].hit_pts;
    }

    level_draw_brick(row, col);
    return pts;
}

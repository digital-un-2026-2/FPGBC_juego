#include "levels.h"
#include "tiles.h"

struct Enemy g_enemies[MAX_ENEMIES];
struct Boss  g_boss;
uint8_t      g_enemies_alive;
uint8_t      g_barrier_hp[SHIELD_COUNT][SHIELD_CELLS];

static const struct LevelSpec level_specs[NUM_LEVELS] = {
    { "OLEADA 1",   26U, 72U, 0U, 0U, 0U },
    { "OLEADA 2",   19U, 52U, 0U, 0U, 0U },
    { "BLINDADOS",  17U, 44U, 0U, 0U, 0U },
    { "ASALTO",     14U, 34U, 1U, 1U, 0U },
    { "JEFE FINAL", 16U, 38U, 1U, 0U, 1U }
};

static const uint8_t shield_tx[SHIELD_COUNT] = { 2U, 7U, 11U, 16U };

static void spawn_grid(uint8_t rows, uint8_t cols, const uint8_t *row_types, uint8_t start_x, uint8_t dx)
{
    uint8_t r, c;
    uint8_t idx = 0U;

    for (r = 0U; r < rows; r++) {
        uint8_t t = row_types[r];
        uint8_t hp = (t == EN_ARMORED || t == EN_DIVER) ? 2U : 1U;
        for (c = 0U; c < cols; c++) {
            if (idx >= MAX_ENEMIES) return;
            g_enemies[idx].x = (int16_t)(start_x + (uint16_t)c * dx);
            g_enemies[idx].y = (int16_t)(24U + (uint16_t)r * 12U);
            g_enemies[idx].type = t;
            g_enemies[idx].hp = hp;
            g_enemies[idx].active = 1U;
            g_enemies[idx].mode = 0U;
            g_enemies[idx].vx = 0;
            g_enemies[idx].vy = 0;
            g_enemies_alive++;
            idx++;
        }
    }
}

void level_load(uint8_t level_idx)
{
    static const uint8_t l1_rows[3] = { EN_SHOOTER, EN_GRUNT, EN_GRUNT };
    static const uint8_t l2_rows[4] = { EN_SHOOTER, EN_SHOOTER, EN_GRUNT, EN_GRUNT };
    static const uint8_t l3_rows[4] = { EN_ARMORED, EN_SHOOTER, EN_GRUNT, EN_GRUNT };
    static const uint8_t l4_rows[4] = { EN_DIVER, EN_ARMORED, EN_SHOOTER, EN_SHOOTER };
    static const uint8_t boss_escort_x[6] = { 20U, 130U, 34U, 60U, 92U, 118U };
    static const uint8_t boss_escort_y[6] = { 48U, 48U,  62U, 62U, 62U, 62U  };
    uint8_t i;

    if (level_idx >= NUM_LEVELS) level_idx = 0U;

    for (i = 0U; i < MAX_ENEMIES; i++) {
        g_enemies[i].active = 0U;
    }
    g_enemies_alive = 0U;
    g_boss.active = 0U;

    if (level_idx == 0U) {
        spawn_grid(3U, 5U, l1_rows, 24U, 24U);
    } else if (level_idx == 1U) {
        spawn_grid(4U, 5U, l2_rows, 24U, 24U);
    } else if (level_idx == 2U) {
        spawn_grid(4U, 6U, l3_rows, 20U, 22U);
    } else if (level_idx == 3U) {
        spawn_grid(4U, 6U, l4_rows, 20U, 22U);
    } else {
        g_boss.x = 64;
        g_boss.y = 22;
        g_boss.vx = 1;
        g_boss.hp = 20U;
        g_boss.max_hp = 20U;
        g_boss.active = 1U;
        g_boss.flash_timer = 0U;
        g_boss.fire_cd = 45U;

        for (i = 0U; i < 6U; i++) {
            uint8_t t = (i < 2U) ? EN_ARMORED : EN_SHOOTER;
            g_enemies[i].x = (int16_t)boss_escort_x[i];
            g_enemies[i].y = (int16_t)boss_escort_y[i];
            g_enemies[i].type = t;
            g_enemies[i].hp = (t == EN_ARMORED) ? 2U : 1U;
            g_enemies[i].active = 1U;
            g_enemies[i].mode = 0U;
            g_enemies[i].vx = 0;
            g_enemies[i].vy = 0;
            g_enemies_alive++;
        }
    }
}

const struct LevelSpec *level_get_spec(uint8_t level_idx)
{
    if (level_idx >= NUM_LEVELS) level_idx = 0U;
    return &level_specs[level_idx];
}

uint16_t level_enemy_points(uint8_t type)
{
    switch (type) {
    case EN_GRUNT:   return 10U;
    case EN_SHOOTER: return 20U;
    case EN_ARMORED: return 40U;
    case EN_DIVER:   return 50U;
    default:         return 10U;
    }
}

void barriers_init(void)
{
    uint8_t s, c;
    for (s = 0U; s < SHIELD_COUNT; s++) {
        for (c = 0U; c < SHIELD_CELLS; c++) {
            g_barrier_hp[s][c] = 3U;
        }
    }
}

void barrier_draw_cell(uint8_t shield, uint8_t cell)
{
    uint8_t tx = (uint8_t)(shield_tx[shield] + (cell & 1U));
    uint8_t ty = (uint8_t)(13U + (cell >> 1));
    uint8_t hp = g_barrier_hp[shield][cell];

    if (hp == 0U) {
        gfx_set_tile(tx, ty, gfx_star_tile(tx, ty), PAL_BG_UI);
    } else if (hp >= 3U) {
        gfx_set_tile(tx, ty, (uint8_t)(TILE_SHD_TL + cell), PAL_BG_GREEN);
    } else if (hp == 2U) {
        gfx_set_tile(tx, ty, TILE_SHD_DMG1, PAL_BG_YELLOW);
    } else {
        gfx_set_tile(tx, ty, TILE_SHD_DMG2, PAL_BG_RED);
    }
}

void barriers_draw_all(void)
{
    uint8_t s, c;
    for (s = 0U; s < SHIELD_COUNT; s++) {
        for (c = 0U; c < SHIELD_CELLS; c++) {
            barrier_draw_cell(s, c);
        }
    }
}

static uint8_t interact_barrier_rect(int16_t px, int16_t py, uint8_t w, uint8_t h, uint8_t crush)
{
    uint8_t s, c;
    int16_t rx = px + (int16_t)w - 1;
    int16_t by = py + (int16_t)h - 1;

    if (by < SHIELD_TOP_Y || py >= SHIELD_BOT_Y) return 0U;

    for (s = 0U; s < SHIELD_COUNT; s++) {
        int16_t sx = (int16_t)((uint16_t)shield_tx[s] << 3);
        if (rx < sx || px >= sx + 16) continue;

        for (c = 0U; c < SHIELD_CELLS; c++) {
            int16_t cx, cy;
            if (g_barrier_hp[s][c] == 0U) continue;
            cx = sx + (int16_t)((c & 1U) ? 8 : 0);
            cy = (int16_t)SHIELD_TOP_Y + (int16_t)((c >= 2U) ? 8 : 0);

            if (rx >= cx && px < cx + 8 && by >= cy && py < cy + 8) {
                if (crush) {
                    g_barrier_hp[s][c] = 0U;
                } else {
                    g_barrier_hp[s][c]--;
                }
                barrier_draw_cell(s, c);
                if (!crush) return 1U;
            }
        }
    }
    return 0U;
}

uint8_t barrier_check_hit(int16_t px, int16_t py, uint8_t w, uint8_t h)
{
    return interact_barrier_rect(px, py, w, h, 0U);
}

void barriers_crush_area(int16_t ex, int16_t ey, uint8_t w, uint8_t h)
{
    (void)interact_barrier_rect(ex, ey, w, h, 1U);
}

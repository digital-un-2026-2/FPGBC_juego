#include <gb/gb.h>
#include <gb/cgb.h>
#include "game.h"
#include "levels.h"
#include "sound.h"
#include "tiles.h"

#define FP_SHIFT        4U
#define SPR_PAD_0       0U
#define SPR_BALL        4U
#define SPR_FX_BASE     5U
#define MAX_FX          4U
#define BRICK_AREA_BOT  (BRICK_TOP_Y + BRICK_ROWS * BRICK_H)

struct SpeedEntry {
    int8_t vx_far;
    int8_t vy_far;
    int8_t vx_mid;
    int8_t vy_mid;
    int8_t vx_ctr;
    int8_t vy_ctr;
};

static const struct SpeedEntry speed_table[5] = {
    { 18, 13, 14, 17, 7,  21 }, /* Tier 0 (Nivel 1 base) */
    { 21, 15, 16, 20, 8,  24 }, /* Tier 1 */
    { 24, 17, 18, 23, 9,  27 }, /* Tier 2 (Nivel 2 base) */
    { 26, 19, 20, 25, 10, 30 }, /* Tier 3 (Nivel 3 base) */
    { 29, 20, 22, 27, 11, 33 }  /* Tier 4 (Velocidad max) */
};

struct FxSlot {
    uint8_t x;
    uint8_t y;
    uint8_t timer;
};

uint8_t  g_state;
uint8_t  g_level;
uint8_t  g_lives;
uint8_t  g_speed_tier;
uint16_t g_score;
int16_t  g_paddle_x;
int16_t  g_ball_x_fp;
int16_t  g_ball_y_fp;
int8_t   g_ball_vx_fp;
int8_t   g_ball_vy_fp;
uint8_t  g_ball_px;
uint8_t  g_ball_py;

static uint8_t prev_joy;
static uint8_t menu_opt;
static uint8_t menu_level_sel;
static uint8_t state_timer;
static uint8_t brick_hits_cnt;
static uint8_t paddle_hit_timer;
static struct FxSlot fx_pool[MAX_FX];

static void hide_all_sprites(void)
{
    uint8_t i;
    for (i = 0U; i < 40U; i++) {
        hide_sprite(i);
    }
}

static void clear_fx(void)
{
    uint8_t i;
    for (i = 0U; i < MAX_FX; i++) {
        fx_pool[i].timer = 0U;
        hide_sprite((uint8_t)(SPR_FX_BASE + i));
    }
}

static void spawn_fx(uint8_t row, uint8_t col)
{
    uint8_t i;
    uint8_t best = 0U;
    uint8_t min_t = 255U;

    for (i = 0U; i < MAX_FX; i++) {
        if (fx_pool[i].timer == 0U) {
            best = i;
            break;
        }
        if (fx_pool[i].timer < min_t) {
            min_t = fx_pool[i].timer;
            best = i;
        }
    }

    fx_pool[best].x = (uint8_t)(PLAY_LEFT + (col << 4) + 4U);
    fx_pool[best].y = (uint8_t)(BRICK_TOP_Y + (row << 3));
    fx_pool[best].timer = 12U;
}

static void update_fx_sprites(void)
{
    uint8_t i;
    for (i = 0U; i < MAX_FX; i++) {
        uint8_t spr_id = (uint8_t)(SPR_FX_BASE + i);
        if (fx_pool[i].timer == 0U) {
            hide_sprite(spr_id);
        } else {
            uint8_t tile = (fx_pool[i].timer > 6U) ? SPR_TILE_FX0 : SPR_TILE_FX1;
            set_sprite_tile(spr_id, tile);
            set_sprite_prop(spr_id, PAL_SPR_FX);
            move_sprite(spr_id, (uint8_t)(fx_pool[i].x + 8U), (uint8_t)(fx_pool[i].y + 16U));
            fx_pool[i].timer--;
        }
    }
}

static void draw_paddle_sprites(void)
{
    static const uint8_t pad_tiles[4] = {
        SPR_TILE_PAD_L, SPR_TILE_PAD_M, SPR_TILE_PAD_M, SPR_TILE_PAD_R
    };
    uint8_t i;
    uint8_t py = (uint8_t)(PADDLE_Y + ((paddle_hit_timer > 0U) ? 1U : 0U));
    if (paddle_hit_timer > 0U) paddle_hit_timer--;

    for (i = 0U; i < 4U; i++) {
        uint8_t spr_id = (uint8_t)(SPR_PAD_0 + i);
        set_sprite_tile(spr_id, pad_tiles[i]);
        set_sprite_prop(spr_id, PAL_SPR_PADDLE);
        move_sprite(spr_id, (uint8_t)((uint8_t)g_paddle_x + (i << 3) + 8U), (uint8_t)(py + 16U));
    }
}

static void draw_ball_sprite(void)
{
    set_sprite_tile(SPR_BALL, SPR_TILE_BALL);
    set_sprite_prop(SPR_BALL, PAL_SPR_BALL);
    move_sprite(SPR_BALL, (uint8_t)(g_ball_px + 8U), (uint8_t)(g_ball_py + 16U));
}

static void sync_ball_on_paddle(void)
{
    g_ball_px = (uint8_t)(g_paddle_x + 13);
    g_ball_py = (uint8_t)(PADDLE_Y - BALL_SIZE);
    g_ball_x_fp = (int16_t)((int16_t)g_ball_px << FP_SHIFT);
    g_ball_y_fp = (int16_t)((int16_t)g_ball_py << FP_SHIFT);
}

static void update_paddle_input(uint8_t joy)
{
    if (joy & J_LEFT)  g_paddle_x -= PADDLE_SPEED;
    if (joy & J_RIGHT) g_paddle_x += PADDLE_SPEED;

    if (g_paddle_x < PADDLE_MIN_X) g_paddle_x = PADDLE_MIN_X;
    if (g_paddle_x > PADDLE_MAX_X) g_paddle_x = PADDLE_MAX_X;
}

static void draw_hud_score(void)
{
    gfx_draw_number(3U, 0U, g_score, 5U, PAL_UI);
}

static void draw_hud_lives(void)
{
    uint8_t i;
    for (i = 0U; i < INIT_LIVES; i++) {
        uint8_t tx = (uint8_t)(16U + i);
        if (i < g_lives) {
            gfx_set_tile(tx, 0U, TILE_HEART, PAL_RED);
        } else {
            gfx_set_tile(tx, 0U, TILE_EMPTY, PAL_UI);
        }
    }
}

static void draw_hud_full(void)
{
    gfx_draw_text(1U, 0U, "P:", PAL_UI);
    draw_hud_score();
    gfx_draw_text(9U, 0U, "N:", PAL_UI);
    gfx_draw_number(11U, 0U, (uint16_t)(g_level + 1U), 1U, PAL_UI);
    gfx_draw_text(14U, 0U, "V:", PAL_UI);
    draw_hud_lives();
}

static void clear_field_rows(uint8_t start_ty, uint8_t count)
{
    uint8_t x, y;
    for (y = start_ty; y < (uint8_t)(start_ty + count); y++) {
        for (x = 1U; x < 19U; x++) {
            gfx_set_tile(x, y, TILE_BG_FIELD, PAL_UI);
        }
    }
}

static void draw_title_cursor(void)
{
    gfx_set_tile(5U, 10U, (menu_opt == 0U) ? TILE_CURSOR : TILE_EMPTY, PAL_YELLOW);
    gfx_set_tile(5U, 12U, (menu_opt == 1U) ? TILE_CURSOR : TILE_EMPTY, PAL_YELLOW);
}

static void draw_title_level_num(void)
{
    gfx_draw_number(14U, 12U, (uint16_t)(menu_level_sel + 1U), 1U, PAL_CYAN);
}

static void enter_title(void)
{
    static const uint8_t border_pals[9] = {
        PAL_RED, PAL_ORANGE, PAL_YELLOW, PAL_GREEN, PAL_CYAN,
        PAL_GREEN, PAL_YELLOW, PAL_ORANGE, PAL_RED
    };
    uint8_t c;

    g_state = STATE_TITLE;
    DISPLAY_OFF;
    hide_all_sprites();
    clear_fx();
    gfx_clear_screen(TILE_EMPTY, PAL_UI);

    for (c = 0U; c < BRICK_COLS; c++) {
        uint8_t tx = (uint8_t)(1U + (c << 1));
        uint8_t pal = border_pals[c];
        gfx_set_tile(tx, 1U, TILE_BRICK1_L, pal);
        gfx_set_tile((uint8_t)(tx + 1U), 1U, TILE_BRICK1_R, pal);
        gfx_set_tile(tx, 16U, TILE_BRICK1_L, pal);
        gfx_set_tile((uint8_t)(tx + 1U), 16U, TILE_BRICK1_R, pal);
    }

    gfx_draw_text(6U, 4U, "ARKANOID", PAL_YELLOW);
    gfx_draw_text(2U, 6U, "BREAKOUT ARCADE", PAL_CYAN);
    gfx_draw_text(7U, 10U, "JUGAR", PAL_UI);
    gfx_draw_text(7U, 12U, "NIVEL:", PAL_UI);
    draw_title_level_num();
    draw_title_cursor();
    gfx_draw_text(2U, 14U, "A O START: JUGAR", PAL_UI);

    SHOW_BKG;
    SHOW_SPRITES;
    DISPLAY_ON;
}

static void enter_level_intro(void)
{
    const char *name = level_get_name(g_level);
    uint8_t len = 0U;
    uint8_t tx;

    while (name[len]) len++;
    tx = (uint8_t)((20U - len) >> 1);

    g_state = STATE_LEVEL_INTRO;
    state_timer = 75U;

    DISPLAY_OFF;
    hide_all_sprites();
    clear_fx();
    gfx_clear_screen(TILE_EMPTY, PAL_UI);

    gfx_draw_text(6U, 6U, "NIVEL", PAL_YELLOW);
    gfx_draw_number(12U, 6U, (uint16_t)(g_level + 1U), 1U, PAL_YELLOW);
    gfx_draw_text(tx, 9U, name, PAL_CYAN);
    gfx_draw_text(5U, 12U, "PREPARADO!", PAL_UI);

    SHOW_BKG;
    SHOW_SPRITES;
    DISPLAY_ON;

    level_load(g_level);
}

static void start_serve_round(void)
{
    g_paddle_x = 64;
    paddle_hit_timer = 0U;
    sync_ball_on_paddle();
    g_ball_vx_fp = 0;
    g_ball_vy_fp = 0;
    g_state = STATE_SERVE;
    gfx_draw_text(5U, 12U, "A: LANZAR", PAL_YELLOW);
    draw_paddle_sprites();
    draw_ball_sprite();
}

static void enter_playfield(void)
{
    g_speed_tier = g_level;
    if (g_speed_tier > 3U) g_speed_tier = 3U;
    brick_hits_cnt = 0U;

    DISPLAY_OFF;
    hide_all_sprites();
    clear_fx();
    level_draw_playfield();
    draw_hud_full();
    start_serve_round();

    SHOW_BKG;
    SHOW_SPRITES;
    DISPLAY_ON;
}

static void start_new_game(uint8_t start_level)
{
    g_level = (start_level < NUM_LEVELS) ? start_level : 0U;
    g_lives = INIT_LIVES;
    g_score = 0U;
    enter_level_intro();
}

static void enter_end_screen(uint8_t is_victory)
{
    g_state = is_victory ? STATE_VICTORY : STATE_GAME_OVER;

    DISPLAY_OFF;
    hide_all_sprites();
    clear_fx();
    gfx_clear_screen(TILE_EMPTY, PAL_UI);

    if (is_victory) {
        gfx_draw_text(5U, 3U, "VICTORIA!", PAL_YELLOW);
        gfx_draw_text(2U, 5U, "JUEGO COMPLETADO", PAL_GREEN);
        gfx_draw_text(4U, 8U, "PUNTOS:", PAL_UI);
        gfx_draw_number(11U, 8U, g_score, 5U, PAL_CYAN);
        gfx_draw_text(1U, 12U, "A: JUGAR OTRA VEZ", PAL_UI);
    } else {
        gfx_draw_text(5U, 3U, "GAME OVER", PAL_RED);
        gfx_draw_text(4U, 6U, "PUNTOS:", PAL_UI);
        gfx_draw_number(11U, 6U, g_score, 5U, PAL_CYAN);
        gfx_draw_text(6U, 8U, "NIVEL:", PAL_UI);
        gfx_draw_number(13U, 8U, (uint16_t)(g_level + 1U), 1U, PAL_YELLOW);
        gfx_draw_text(3U, 12U, "A: REINTENTAR", PAL_UI);
    }
    gfx_draw_text(4U, 14U, "SELECT: MENU", PAL_UI);

    SHOW_BKG;
    SHOW_SPRITES;
    DISPLAY_ON;

    sound_play(is_victory ? SFX_VICTORY : SFX_GAME_OVER);
}

static uint8_t apply_brick_hit(uint8_t row, uint8_t col)
{
    uint8_t destroyed = 0U;
    uint16_t pts = level_hit_brick(row, col, &destroyed);
    uint32_t next_score;

    if (pts > 0U) {
        next_score = (uint32_t)g_score + pts;
        g_score = (next_score > 65000UL) ? 65000U : (uint16_t)next_score;
        draw_hud_score();
    }

    spawn_fx(row, col);
    brick_hits_cnt++;
    if (brick_hits_cnt >= 10U) {
        brick_hits_cnt = 0U;
        if (g_speed_tier < 4U) g_speed_tier++;
    }

    sound_play(destroyed ? SFX_BRICK_BREAK : SFX_BRICK_HIT);

    if (g_bricks_left == 0U) {
        g_state = STATE_LEVEL_CLEAR;
        state_timer = 90U;
        hide_sprite(SPR_BALL);
        gfx_draw_text(1U, 12U, "NIVEL COMPLETADO!", PAL_YELLOW);
        sound_play(SFX_LEVEL_CLEAR);
        return 1U;
    }
    return 0U;
}

static uint8_t check_brick_axis_x(int16_t *bx, int16_t by, uint8_t *level_cleared)
{
    int16_t rx, lx, y0, y1;
    uint8_t col, r0, r1, r;

    *level_cleared = 0U;
    y0 = by;
    y1 = by + (int16_t)BALL_SIZE - 1;
    if (y1 < (int16_t)BRICK_TOP_Y || y0 >= (int16_t)BRICK_AREA_BOT) return 0U;

    if (y0 < (int16_t)BRICK_TOP_Y) y0 = BRICK_TOP_Y;
    if (y1 >= (int16_t)BRICK_AREA_BOT) y1 = (int16_t)(BRICK_AREA_BOT - 1);
    r0 = (uint8_t)((uint16_t)(y0 - (int16_t)BRICK_TOP_Y) >> 3);
    r1 = (uint8_t)((uint16_t)(y1 - (int16_t)BRICK_TOP_Y) >> 3);

    if (g_ball_vx_fp > 0) {
        rx = *bx + (int16_t)BALL_SIZE - 1;
        if (rx < (int16_t)PLAY_LEFT || rx >= (int16_t)PLAY_RIGHT) return 0U;
        col = (uint8_t)((uint16_t)(rx - (int16_t)PLAY_LEFT) >> 4);
        for (r = r0; r <= r1; r++) {
            if (brick_hp[r][col] > 0U) {
                *bx = (int16_t)(PLAY_LEFT + (col << 4) - BALL_SIZE);
                g_ball_x_fp = (int16_t)(*bx << FP_SHIFT);
                g_ball_vx_fp = (int8_t)-g_ball_vx_fp;
                *level_cleared = apply_brick_hit(r, col);
                return 1U;
            }
        }
    } else if (g_ball_vx_fp < 0) {
        lx = *bx;
        if (lx < (int16_t)PLAY_LEFT || lx >= (int16_t)PLAY_RIGHT) return 0U;
        col = (uint8_t)((uint16_t)(lx - (int16_t)PLAY_LEFT) >> 4);
        for (r = r0; r <= r1; r++) {
            if (brick_hp[r][col] > 0U) {
                *bx = (int16_t)(PLAY_LEFT + ((col + 1U) << 4));
                g_ball_x_fp = (int16_t)(*bx << FP_SHIFT);
                g_ball_vx_fp = (int8_t)-g_ball_vx_fp;
                *level_cleared = apply_brick_hit(r, col);
                return 1U;
            }
        }
    }
    return 0U;
}

static uint8_t check_brick_axis_y(int16_t bx, int16_t *by, uint8_t *level_cleared)
{
    int16_t x0, x1, ty_px;
    uint8_t row, c0, c1, c;

    *level_cleared = 0U;
    x0 = bx;
    x1 = bx + (int16_t)BALL_SIZE - 1;
    if (x1 < (int16_t)PLAY_LEFT || x0 >= (int16_t)PLAY_RIGHT) return 0U;

    if (x0 < (int16_t)PLAY_LEFT) x0 = PLAY_LEFT;
    if (x1 >= (int16_t)PLAY_RIGHT) x1 = (int16_t)(PLAY_RIGHT - 1);
    c0 = (uint8_t)((uint16_t)(x0 - (int16_t)PLAY_LEFT) >> 4);
    c1 = (uint8_t)((uint16_t)(x1 - (int16_t)PLAY_LEFT) >> 4);

    if (g_ball_vy_fp > 0) {
        ty_px = *by + (int16_t)BALL_SIZE - 1;
        if (ty_px < (int16_t)BRICK_TOP_Y || ty_px >= (int16_t)BRICK_AREA_BOT) return 0U;
        row = (uint8_t)((uint16_t)(ty_px - (int16_t)BRICK_TOP_Y) >> 3);
        for (c = c0; c <= c1; c++) {
            if (brick_hp[row][c] > 0U) {
                *by = (int16_t)(BRICK_TOP_Y + (row << 3) - BALL_SIZE);
                g_ball_y_fp = (int16_t)(*by << FP_SHIFT);
                g_ball_vy_fp = (int8_t)-g_ball_vy_fp;
                *level_cleared = apply_brick_hit(row, c);
                return 1U;
            }
        }
    } else if (g_ball_vy_fp < 0) {
        ty_px = *by;
        if (ty_px < (int16_t)BRICK_TOP_Y || ty_px >= (int16_t)BRICK_AREA_BOT) return 0U;
        row = (uint8_t)((uint16_t)(ty_px - (int16_t)BRICK_TOP_Y) >> 3);
        for (c = c0; c <= c1; c++) {
            if (brick_hp[row][c] > 0U) {
                *by = (int16_t)(BRICK_TOP_Y + ((row + 1U) << 3));
                g_ball_y_fp = (int16_t)(*by << FP_SHIFT);
                g_ball_vy_fp = (int8_t)-g_ball_vy_fp;
                *level_cleared = apply_brick_hit(row, c);
                return 1U;
            }
        }
    }
    return 0U;
}

static void update_playing(uint8_t joy, uint8_t pressed)
{
    int16_t bx, by;
    uint8_t wall_hit = 0U;
    uint8_t brick_hit = 0U;
    uint8_t cleared = 0U;

    if (pressed & J_START) {
        g_state = STATE_PAUSED;
        gfx_draw_text(7U, 12U, "PAUSA", PAL_YELLOW);
        gfx_draw_text(4U, 13U, "START:SEGUIR", PAL_UI);
        gfx_draw_text(6U, 14U, "SEL:MENU", PAL_UI);
        sound_play(SFX_PAUSE);
        return;
    }

    update_paddle_input(joy);
    draw_paddle_sprites();

    /* Paso horizontal X */
    by = g_ball_y_fp >> FP_SHIFT;
    g_ball_x_fp += g_ball_vx_fp;
    bx = g_ball_x_fp >> FP_SHIFT;

    if (bx < (int16_t)PLAY_LEFT) {
        bx = PLAY_LEFT;
        g_ball_x_fp = (int16_t)(bx << FP_SHIFT);
        if (g_ball_vx_fp < 0) g_ball_vx_fp = (int8_t)-g_ball_vx_fp;
        wall_hit = 1U;
    } else if (bx > (int16_t)(PLAY_RIGHT - BALL_SIZE)) {
        bx = (int16_t)(PLAY_RIGHT - BALL_SIZE);
        g_ball_x_fp = (int16_t)(bx << FP_SHIFT);
        if (g_ball_vx_fp > 0) g_ball_vx_fp = (int8_t)-g_ball_vx_fp;
        wall_hit = 1U;
    }

    brick_hit = check_brick_axis_x(&bx, by, &cleared);
    if (cleared) return;

    /* Paso vertical Y */
    g_ball_y_fp += g_ball_vy_fp;
    by = g_ball_y_fp >> FP_SHIFT;

    if (by < (int16_t)PLAY_TOP) {
        by = PLAY_TOP;
        g_ball_y_fp = (int16_t)(by << FP_SHIFT);
        if (g_ball_vy_fp < 0) g_ball_vy_fp = (int8_t)-g_ball_vy_fp;
        wall_hit = 1U;
    }

    if (check_brick_axis_y(bx, &by, &cleared)) {
        brick_hit = 1U;
        if (cleared) return;
    }

    /* Rebote con la barra */
    if (g_ball_vy_fp > 0) {
        if (by + (int16_t)BALL_SIZE >= (int16_t)PADDLE_Y &&
            by <= (int16_t)(PADDLE_Y + 4U) &&
            bx + (int16_t)BALL_SIZE > g_paddle_x &&
            bx < g_paddle_x + (int16_t)PADDLE_W) {
            const struct SpeedEntry *sp = &speed_table[g_speed_tier];
            int16_t rel = (bx + 3) - g_paddle_x;

            by = (int16_t)(PADDLE_Y - BALL_SIZE);
            g_ball_y_fp = (int16_t)(by << FP_SHIFT);

            if (rel < 6) {
                g_ball_vx_fp = (int8_t)-sp->vx_far;
                g_ball_vy_fp = (int8_t)-sp->vy_far;
            } else if (rel < 13) {
                g_ball_vx_fp = (int8_t)-sp->vx_mid;
                g_ball_vy_fp = (int8_t)-sp->vy_mid;
            } else if (rel < 19) {
                g_ball_vx_fp = (rel < 16) ? (int8_t)-sp->vx_ctr : sp->vx_ctr;
                g_ball_vy_fp = (int8_t)-sp->vy_ctr;
            } else if (rel < 26) {
                g_ball_vx_fp = sp->vx_mid;
                g_ball_vy_fp = (int8_t)-sp->vy_mid;
            } else {
                g_ball_vx_fp = sp->vx_far;
                g_ball_vy_fp = (int8_t)-sp->vy_far;
            }

            paddle_hit_timer = 4U;
            sound_play(SFX_PADDLE);
            wall_hit = 0U;
        }
    }

    /* Caida por debajo de la pantalla */
    if (by >= (int16_t)PLAY_BOTTOM) {
        if (g_lives > 0U) g_lives--;
        draw_hud_lives();
        hide_sprite(SPR_BALL);
        sound_play(SFX_LOSE_LIFE);
        g_state = STATE_LIFE_LOST;
        state_timer = 45U;
        return;
    }

    if (wall_hit && !brick_hit) {
        sound_play(SFX_WALL);
    }

    g_ball_px = (uint8_t)bx;
    g_ball_py = (uint8_t)by;
    draw_ball_sprite();
}

void game_init(void)
{
    prev_joy = 0U;
    menu_opt = 0U;
    menu_level_sel = 0U;
    g_score = 0U;
    g_lives = INIT_LIVES;
    g_level = 0U;
    enter_title();
}

void game_update(void)
{
    uint8_t joy = joypad();
    uint8_t pressed = (uint8_t)(joy & (uint8_t)~prev_joy);
    prev_joy = joy;

    update_fx_sprites();

    switch (g_state) {
    case STATE_TITLE:
        if (pressed & (J_UP | J_DOWN)) {
            menu_opt ^= 1U;
            draw_title_cursor();
            sound_play(SFX_MENU);
        }
        if ((menu_opt == 1U) && (pressed & (J_LEFT | J_RIGHT))) {
            if (pressed & J_RIGHT) {
                menu_level_sel = (uint8_t)((menu_level_sel + 1U) % NUM_LEVELS);
            } else {
                menu_level_sel = (menu_level_sel == 0U) ? (uint8_t)(NUM_LEVELS - 1U) : (uint8_t)(menu_level_sel - 1U);
            }
            draw_title_level_num();
            sound_play(SFX_MENU);
        }
        if (pressed & (J_A | J_START)) {
            sound_play(SFX_LAUNCH);
            start_new_game(menu_level_sel);
        }
        break;

    case STATE_LEVEL_INTRO:
        if (state_timer > 0U) state_timer--;
        if (state_timer == 0U || (pressed & (J_A | J_START))) {
            enter_playfield();
        }
        break;

    case STATE_SERVE:
        if (pressed & J_START) {
            clear_field_rows(12U, 1U);
            g_state = STATE_PAUSED;
            gfx_draw_text(7U, 12U, "PAUSA", PAL_YELLOW);
            gfx_draw_text(4U, 13U, "START:SEGUIR", PAL_UI);
            gfx_draw_text(6U, 14U, "SEL:MENU", PAL_UI);
            sound_play(SFX_PAUSE);
            break;
        }
        update_paddle_input(joy);
        draw_paddle_sprites();
        sync_ball_on_paddle();
        draw_ball_sprite();

        if (pressed & J_A) {
            const struct SpeedEntry *sp = &speed_table[g_speed_tier];
            clear_field_rows(12U, 1U);
            if (joy & J_LEFT) {
                g_ball_vx_fp = (int8_t)-sp->vx_mid;
            } else {
                g_ball_vx_fp = sp->vx_mid;
            }
            g_ball_vy_fp = (int8_t)-sp->vy_mid;
            g_state = STATE_PLAYING;
            sound_play(SFX_LAUNCH);
        }
        break;

    case STATE_PLAYING:
        update_playing(joy, pressed);
        break;

    case STATE_PAUSED:
        if (pressed & J_START) {
            clear_field_rows(12U, 3U);
            if (g_ball_vy_fp == 0 && g_ball_vx_fp == 0) {
                g_state = STATE_SERVE;
                gfx_draw_text(5U, 12U, "A: LANZAR", PAL_YELLOW);
            } else {
                g_state = STATE_PLAYING;
            }
            sound_play(SFX_MENU);
        } else if (pressed & J_SELECT) {
            enter_title();
        }
        break;

    case STATE_LIFE_LOST:
        if (state_timer > 0U) state_timer--;
        if (state_timer == 0U) {
            if (g_lives == 0U) {
                enter_end_screen(0U);
            } else {
                start_serve_round();
            }
        }
        break;

    case STATE_LEVEL_CLEAR:
        if (state_timer > 0U) state_timer--;
        if (state_timer == 0U || (pressed & (J_A | J_START))) {
            g_level++;
            if (g_level >= NUM_LEVELS) {
                enter_end_screen(1U);
            } else {
                enter_level_intro();
            }
        }
        break;

    case STATE_GAME_OVER:
    case STATE_VICTORY:
        if (pressed & (J_A | J_START)) {
            sound_play(SFX_LAUNCH);
            start_new_game(0U);
        } else if (pressed & J_SELECT) {
            enter_title();
        }
        break;

    default:
        enter_title();
        break;
    }
}
